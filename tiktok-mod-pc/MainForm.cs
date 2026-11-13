using System.Diagnostics;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace TikTokModPC;

public sealed class MainForm : Form
{
    private const string ApkUrl = "https://github.com/namecallfilter/tiktokmodcloud/releases/download/v46.4.3-mod-fix/46.4.3_universal_fix.apk";
    private const string ApkFileName = "46.4.3_universal_fix.apk";
    private const long ExpectedApkSize = 501_809_029;
    private const string ExpectedSha256 = "88845094adb702e2b92ada1fec56920083208ff48db03a879621f3de8a14190c";
    private const string DefaultPackageName = "com.zhiliaoapp.musically";

    private static readonly HttpClient Http = new()
    {
        Timeout = Timeout.InfiniteTimeSpan
    };

    private readonly string _dataDirectory =
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "TikTokModPC");

    private readonly Button _downloadButton = new();
    private readonly Button _chooseAdbButton = new();
    private readonly Button _refreshButton = new();
    private readonly Button _installButton = new();
    private readonly Button _launchButton = new();
    private readonly Button _openFolderButton = new();
    private readonly TextBox _adbPathBox = new();
    private readonly TextBox _packageBox = new();
    private readonly ComboBox _deviceBox = new();
    private readonly ProgressBar _progress = new();
    private readonly Label _apkState = new();
    private readonly Label _status = new();
    private readonly TextBox _log = new();

    private string _adbPath = "";
    private string ApkPath => Path.Combine(_dataDirectory, ApkFileName);

    public MainForm()
    {
        Text = "TikTok Mod PC";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(760, 720);
        Size = new Size(820, 790);
        BackColor = Color.FromArgb(13, 16, 22);
        ForeColor = Color.FromArgb(235, 240, 248);
        Font = new Font("Segoe UI", 9.5F);
        AutoScaleMode = AutoScaleMode.Font;

        BuildUi();
        LoadSettings();
        _apkState.Text = File.Exists(ApkPath) ? "APK уже загружен — проверка перед установкой" : "APK ещё не загружен · около 478,5 МиБ";
        _status.Text = "Открой Android-эмулятор, затем проверь подключение ADB.";
        Log("TikTok Mod PC готов. Это лаунчер Android-приложения, не нативная Windows-версия TikTok.");
        if (!string.IsNullOrWhiteSpace(_adbPath))
            Log($"Найден ADB: {_adbPath}");
    }

    private void BuildUi()
    {
        var page = new Panel
        {
            Dock = DockStyle.Fill,
            AutoScroll = true,
            Padding = new Padding(24),
            BackColor = BackColor
        };
        Controls.Add(page);

        var title = new Label
        {
            Text = "TikTok Mod PC",
            Font = new Font("Segoe UI Semibold", 25F, FontStyle.Bold),
            ForeColor = Color.White,
            Location = new Point(24, 18),
            Size = new Size(650, 42)
        };
        page.Controls.Add(title);

        var subtitle = new Label
        {
            Text = "ANDROID BUILD 46.4.3  ·  WINDOWS LAUNCHER",
            ForeColor = Color.FromArgb(135, 151, 174),
            Location = new Point(27, 62),
            Size = new Size(650, 24)
        };
        page.Controls.Add(subtitle);

        _status.Location = new Point(27, 98);
        _status.Size = new Size(700, 28);
        _status.ForeColor = Color.FromArgb(112, 225, 185);
        page.Controls.Add(_status);

        var apkGroup = MakeGroup("1  ·  ФАЙЛ ПРИЛОЖЕНИЯ", 27, 140, 720, 145);
        page.Controls.Add(apkGroup);

        _apkState.Location = new Point(18, 31);
        _apkState.Size = new Size(670, 24);
        _apkState.ForeColor = Color.FromArgb(195, 207, 225);
        apkGroup.Controls.Add(_apkState);

        StyleButton(_downloadButton, "Скачать APK", 18, 70, 170, 40, primary: true);
        _downloadButton.Click += async (_, _) => await DownloadApkAsync();
        apkGroup.Controls.Add(_downloadButton);

        _progress.Location = new Point(205, 78);
        _progress.Size = new Size(475, 22);
        _progress.Minimum = 0;
        _progress.Maximum = 1000;
        _progress.Value = 0;
        _progress.Style = ProgressBarStyle.Continuous;
        apkGroup.Controls.Add(_progress);

        var hashNote = new Label
        {
            Text = "SHA-256 проверяется после загрузки; APK не вшит в EXE.",
            Location = new Point(205, 105),
            Size = new Size(480, 22),
            ForeColor = Color.FromArgb(125, 143, 166)
        };
        apkGroup.Controls.Add(hashNote);

        var emulatorGroup = MakeGroup("2  ·  ANDROID-ЭМУЛЯТОР", 27, 300, 720, 210);
        page.Controls.Add(emulatorGroup);

        var adbLabel = new Label
        {
            Text = "ADB / HD-Adb:",
            Location = new Point(18, 31),
            Size = new Size(110, 24),
            ForeColor = Color.FromArgb(180, 193, 212)
        };
        emulatorGroup.Controls.Add(adbLabel);

        _adbPathBox.Location = new Point(132, 28);
        _adbPathBox.Size = new Size(425, 28);
        _adbPathBox.ReadOnly = true;
        StyleTextBox(_adbPathBox);
        emulatorGroup.Controls.Add(_adbPathBox);

        StyleButton(_chooseAdbButton, "Обзор…", 570, 25, 110, 34);
        _chooseAdbButton.Click += (_, _) => ChooseAdb();
        emulatorGroup.Controls.Add(_chooseAdbButton);

        var deviceLabel = new Label
        {
            Text = "Устройство:",
            Location = new Point(18, 76),
            Size = new Size(110, 24),
            ForeColor = Color.FromArgb(180, 193, 212)
        };
        emulatorGroup.Controls.Add(deviceLabel);

        _deviceBox.Location = new Point(132, 72);
        _deviceBox.Size = new Size(425, 30);
        _deviceBox.DropDownStyle = ComboBoxStyle.DropDownList;
        _deviceBox.BackColor = Color.FromArgb(24, 30, 40);
        _deviceBox.ForeColor = Color.White;
        emulatorGroup.Controls.Add(_deviceBox);

        StyleButton(_refreshButton, "Проверить ADB", 570, 68, 110, 36);
        _refreshButton.Click += async (_, _) => await RefreshDevicesAsync();
        emulatorGroup.Controls.Add(_refreshButton);

        var packageLabel = new Label
        {
            Text = "Пакет приложения:",
            Location = new Point(18, 119),
            Size = new Size(115, 24),
            ForeColor = Color.FromArgb(180, 193, 212)
        };
        emulatorGroup.Controls.Add(packageLabel);

        _packageBox.Location = new Point(132, 115);
        _packageBox.Size = new Size(548, 28);
        _packageBox.Text = DefaultPackageName;
        StyleTextBox(_packageBox);
        emulatorGroup.Controls.Add(_packageBox);

        StyleButton(_installButton, "Установить / обновить", 132, 158, 235, 38, primary: true);
        _installButton.Click += async (_, _) => await InstallApkAsync();
        emulatorGroup.Controls.Add(_installButton);

        StyleButton(_launchButton, "Запустить TikTok Mod", 380, 158, 300, 38);
        _launchButton.Click += async (_, _) => await LaunchAppAsync();
        emulatorGroup.Controls.Add(_launchButton);

        var logGroup = MakeGroup("ЖУРНАЛ", 27, 525, 720, 145);
        page.Controls.Add(logGroup);

        _log.Location = new Point(14, 27);
        _log.Size = new Size(690, 103);
        _log.Multiline = true;
        _log.ReadOnly = true;
        _log.ScrollBars = ScrollBars.Vertical;
        _log.BackColor = Color.FromArgb(8, 11, 16);
        _log.ForeColor = Color.FromArgb(175, 194, 215);
        _log.BorderStyle = BorderStyle.FixedSingle;
        _log.Font = new Font("Consolas", 9F);
        logGroup.Controls.Add(_log);

        StyleButton(_openFolderButton, "Открыть папку APK", 27, 685, 190, 36);
        _openFolderButton.Click += (_, _) =>
        {
            Directory.CreateDirectory(_dataDirectory);
            Process.Start(new ProcessStartInfo("explorer.exe", _dataDirectory) { UseShellExecute = true });
        };
        page.Controls.Add(_openFolderButton);

        var warning = new Label
        {
            Text = "ВНИМАНИЕ: это сторонняя модификация, её безопасность не проверена. Не вводи пароль от основного аккаунта. Приложение работает внутри установленного Android-эмулятора.",
            Location = new Point(27, 735),
            Size = new Size(700, 50),
            ForeColor = Color.FromArgb(232, 180, 105),
            AutoSize = false
        };
        page.Controls.Add(warning);
    }

    private static GroupBox MakeGroup(string title, int x, int y, int width, int height) => new()
    {
        Text = title,
        Location = new Point(x, y),
        Size = new Size(width, height),
        ForeColor = Color.FromArgb(150, 174, 204),
        BackColor = Color.FromArgb(19, 24, 33),
        Font = new Font("Segoe UI Semibold", 9F, FontStyle.Bold),
        Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right
    };

    private static void StyleButton(Button button, string text, int x, int y, int width, int height, bool primary = false)
    {
        button.Text = text;
        button.Location = new Point(x, y);
        button.Size = new Size(width, height);
        button.FlatStyle = FlatStyle.Flat;
        button.FlatAppearance.BorderSize = primary ? 0 : 1;
        button.FlatAppearance.BorderColor = Color.FromArgb(60, 74, 94);
        button.BackColor = primary ? Color.FromArgb(83, 204, 163) : Color.FromArgb(32, 41, 55);
        button.ForeColor = primary ? Color.FromArgb(9, 25, 22) : Color.FromArgb(236, 242, 250);
        button.Font = new Font("Segoe UI Semibold", 9F, FontStyle.Bold);
        button.Cursor = Cursors.Hand;
    }

    private static void StyleTextBox(TextBox box)
    {
        box.BackColor = Color.FromArgb(24, 30, 40);
        box.ForeColor = Color.White;
        box.BorderStyle = BorderStyle.FixedSingle;
    }

    private void LoadSettings()
    {
        try
        {
            var settingsPath = Path.Combine(_dataDirectory, "settings.json");
            if (File.Exists(settingsPath))
            {
                var settings = JsonSerializer.Deserialize<AppSettings>(File.ReadAllText(settingsPath));
                if (settings is not null)
                {
                    _adbPath = settings.AdbPath ?? "";
                    if (File.Exists(_adbPath)) _adbPathBox.Text = _adbPath;
                }
            }
        }
        catch (Exception ex)
        {
            Log($"Не удалось прочитать настройки: {ex.Message}");
        }

        if (string.IsNullOrWhiteSpace(_adbPath) || !File.Exists(_adbPath))
        {
            _adbPath = FindAdb() ?? "";
            _adbPathBox.Text = _adbPath;
        }
    }

    private void SaveSettings()
    {
        try
        {
            Directory.CreateDirectory(_dataDirectory);
            File.WriteAllText(
                Path.Combine(_dataDirectory, "settings.json"),
                JsonSerializer.Serialize(new AppSettings { AdbPath = _adbPath }, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch (Exception ex)
        {
            Log($"Не удалось сохранить настройки: {ex.Message}");
        }
    }

    private static string? FindAdb()
    {
        var candidates = new List<string>();
        var local = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        candidates.Add(Path.Combine(local, "Android", "Sdk", "platform-tools", "adb.exe"));
        candidates.Add(@"C:\Program Files\BlueStacks_nxt\HD-Adb.exe");
        candidates.Add(@"C:\Program Files\BlueStacks\HD-Adb.exe");
        candidates.Add(@"C:\Program Files\LDPlayer\LDPlayer9\adb.exe");
        candidates.Add(@"C:\LDPlayer\LDPlayer9\adb.exe");

        foreach (var variable in new[] { "ANDROID_HOME", "ANDROID_SDK_ROOT" })
        {
            var value = Environment.GetEnvironmentVariable(variable);
            if (!string.IsNullOrWhiteSpace(value))
                candidates.Add(Path.Combine(value, "platform-tools", "adb.exe"));
        }

        var pathValue = Environment.GetEnvironmentVariable("PATH") ?? "";
        foreach (var directory in pathValue.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
            candidates.Add(Path.Combine(directory.Trim(), "adb.exe"));

        return candidates.FirstOrDefault(File.Exists);
    }

    private void ChooseAdb()
    {
        using var dialog = new OpenFileDialog
        {
            Title = "Выбери adb.exe или HD-Adb.exe",
            Filter = "Android Debug Bridge|adb.exe;HD-Adb.exe|Все файлы|*.*",
            CheckFileExists = true
        };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        _adbPath = dialog.FileName;
        _adbPathBox.Text = _adbPath;
        SaveSettings();
        Log($"Выбран ADB: {_adbPath}");
        SetStatus("ADB выбран. Нажми «Проверить ADB» после запуска эмулятора.");
    }

    private async Task DownloadApkAsync()
    {
        SetBusy(true);
        _progress.Value = 0;
        var tempPath = ApkPath + ".download";
        try
        {
            Directory.CreateDirectory(_dataDirectory);
            if (File.Exists(tempPath)) File.Delete(tempPath);

            Log($"Загрузка {ApkFileName} (~{ExpectedApkSize / 1024d / 1024d:F1} МиБ)...");
            SetStatus("Загрузка APK…");

            using var cts = new CancellationTokenSource(TimeSpan.FromMinutes(30));
            using var request = new HttpRequestMessage(HttpMethod.Get, ApkUrl);
            request.Headers.UserAgent.ParseAdd("TikTokModPC/1.0");
            using var response = await Http.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cts.Token);
            response.EnsureSuccessStatusCode();

            var contentType = response.Content.Headers.ContentType?.MediaType ?? "";
            if (contentType.Contains("text/html", StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Сервер вернул HTML вместо APK.");

            using var sha = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
            long total = 0;
            var buffer = new byte[1024 * 1024];

            await using (var input = await response.Content.ReadAsStreamAsync(cts.Token))
            await using (var output = new FileStream(tempPath, FileMode.CreateNew, FileAccess.Write, FileShare.None, 1024 * 1024, true))
            {
                int read;
                while ((read = await input.ReadAsync(buffer.AsMemory(0, buffer.Length), cts.Token)) > 0)
                {
                    total += read;
                    if (total > ExpectedApkSize)
                        throw new InvalidDataException("Файл больше размера, указанного в метаданных релиза.");
                    await output.WriteAsync(buffer.AsMemory(0, read), cts.Token);
                    sha.AppendData(buffer, 0, read);
                    _progress.Value = Math.Clamp((int)(total * 1000 / ExpectedApkSize), 0, 1000);
                    _apkState.Text = $"Загружено {total / 1024d / 1024d:F1} / {ExpectedApkSize / 1024d / 1024d:F1} МиБ";
                }
                await output.FlushAsync(cts.Token);
            }

            var actualHash = Convert.ToHexString(sha.GetHashAndReset()).ToLowerInvariant();
            if (total != ExpectedApkSize)
                throw new InvalidDataException($"Размер не совпал: получено {total}, ожидалось {ExpectedApkSize} байт.");
            if (!CryptographicOperations.FixedTimeEquals(
                    Encoding.ASCII.GetBytes(actualHash),
                    Encoding.ASCII.GetBytes(ExpectedSha256)))
                throw new InvalidDataException("SHA-256 не совпал. Файл удалён, установка запрещена.");

            if (File.Exists(ApkPath)) File.Delete(ApkPath);
            File.Move(tempPath, ApkPath);
            _progress.Value = 1000;
            _apkState.Text = "APK загружен и совпадает с SHA-256 релиза.";
            Log($"Загрузка завершена. SHA-256: {actualHash}");
            SetStatus("APK проверен по контрольной сумме. Теперь подключи эмулятор.");
        }
        catch (Exception ex)
        {
            try { if (File.Exists(tempPath)) File.Delete(tempPath); } catch { }
            Log($"Ошибка загрузки: {ex.Message}");
            SetStatus("Загрузка не завершена.", true);
            MessageBox.Show(this, ex.Message, "Не удалось скачать APK", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            SetBusy(false);
        }
    }

    private Task<string> EnsureAdbAsync()
    {
        if (File.Exists(_adbPath)) return Task.FromResult(_adbPath);
        _adbPath = FindAdb() ?? "";
        if (!File.Exists(_adbPath))
            throw new FileNotFoundException("ADB не найден. Установи Android-эмулятор с ADB или укажи adb.exe вручную.");
        _adbPathBox.Text = _adbPath;
        SaveSettings();
        return Task.FromResult(_adbPath);
    }

    private async Task RefreshDevicesAsync()
    {
        try
        {
            var adb = await EnsureAdbAsync();
            var result = await RunAdbAsync(adb, ["devices"]);
            if (result.ExitCode != 0) throw new InvalidOperationException(result.Error);
            var devices = result.Output.Split('\n', StringSplitOptions.RemoveEmptyEntries)
                .Skip(1)
                .Select(line => line.Trim().Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries))
                .Where(parts => parts.Length >= 2 && parts[1] == "device")
                .Select(parts => new DeviceEntry(parts[0]))
                .ToList();

            _deviceBox.DataSource = null;
            _deviceBox.DataSource = devices;
            if (devices.Count > 0)
            {
                _deviceBox.SelectedIndex = 0;
                Log($"ADB: найдено устройств — {devices.Count}. Первое: {devices[0].Id}");
                SetStatus("Эмулятор подключён. Можно устанавливать APK.");
            }
            else
            {
                Log("ADB не видит активное устройство. Открой эмулятор и включи ADB debugging.");
                SetStatus("Устройство не найдено. Проверь, что эмулятор запущен и ADB включён.", true);
            }
        }
        catch (Exception ex)
        {
            Log($"ADB: {ex.Message}");
            SetStatus("Не удалось проверить ADB.", true);
            MessageBox.Show(this, ex.Message, "Проблема с ADB", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private async Task InstallApkAsync()
    {
        if (!File.Exists(ApkPath))
        {
            MessageBox.Show(this, "Сначала скачай APK.", "APK отсутствует", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        SetBusy(true);
        try
        {
            await VerifyLocalApkAsync();
            var adb = await EnsureAdbAsync();
            var device = GetSelectedDevice();
            if (device is null)
                throw new InvalidOperationException("Сначала нажми «Проверить ADB» и выбери подключённое устройство.");

            Log($"Установка на {device.Id}…");
            SetStatus("Установка приложения в эмулятор…");
            var result = await RunAdbAsync(adb, ["-s", device.Id, "install", "-r", ApkPath]);
            Log(result.Output);
            if (result.ExitCode != 0 || !result.Output.Contains("Success", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException(string.IsNullOrWhiteSpace(result.Error) ? result.Output : result.Error);

            var detectedPackage = await FindTikTokPackageAsync(adb, device.Id);
            if (!string.IsNullOrWhiteSpace(detectedPackage))
                _packageBox.Text = detectedPackage;

            Log("APK установлен. Нажми «Запустить TikTok Mod».");
            SetStatus("Установка завершена.");
        }
        catch (Exception ex)
        {
            Log($"Ошибка установки: {ex.Message}");
            SetStatus("Не удалось установить APK.", true);
            MessageBox.Show(this, ex.Message, "Ошибка установки", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            SetBusy(false);
        }
    }

    private async Task LaunchAppAsync()
    {
        try
        {
            var adb = await EnsureAdbAsync();
            var device = GetSelectedDevice();
            if (device is null)
                throw new InvalidOperationException("Сначала подключи эмулятор через кнопку «Проверить ADB».");

            var packageName = _packageBox.Text.Trim();
            if (!Regex.IsMatch(packageName, @"^[A-Za-z0-9_]+(\.[A-Za-z0-9_]+)+$"))
                throw new InvalidOperationException("Проверь имя пакета приложения.");

            var result = await RunAdbAsync(adb,
                ["-s", device.Id, "shell", "monkey", "-p", packageName, "-c", "android.intent.category.LAUNCHER", "1"]);
            Log(result.Output);
            if (result.ExitCode != 0 || result.Output.Contains("No activities found", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("Не удалось открыть приложение. Проверь имя пакета и то, что APK установлен.");
            SetStatus("Команда запуска отправлена в Android-эмулятор.");
        }
        catch (Exception ex)
        {
            Log($"Запуск: {ex.Message}");
            SetStatus("Не удалось запустить приложение.", true);
            MessageBox.Show(this, ex.Message, "Ошибка запуска", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private async Task<string?> FindTikTokPackageAsync(string adb, string deviceId)
    {
        var result = await RunAdbAsync(adb, ["-s", deviceId, "shell", "pm", "list", "packages"]);
        if (result.ExitCode != 0) return null;
        return result.Output.Split('\n', StringSplitOptions.RemoveEmptyEntries)
            .Select(line => line.Trim().Replace("package:", "", StringComparison.Ordinal))
            .FirstOrDefault(name =>
                name.Contains("musically", StringComparison.OrdinalIgnoreCase) ||
                name.Contains("tiktok", StringComparison.OrdinalIgnoreCase));
    }

    private async Task VerifyLocalApkAsync()
    {
        await using var stream = File.OpenRead(ApkPath);
        var hash = Convert.ToHexString(await SHA256.HashDataAsync(stream)).ToLowerInvariant();
        if (stream.Length != ExpectedApkSize || !string.Equals(hash, ExpectedSha256, StringComparison.Ordinal))
            throw new InvalidDataException("Локальный APK не прошёл проверку размера или SHA-256. Скачай его заново.");
    }

    private DeviceEntry? GetSelectedDevice() => _deviceBox.SelectedItem as DeviceEntry;

    private static async Task<CommandResult> RunAdbAsync(string adb, IReadOnlyList<string> arguments)
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = adb,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        foreach (var argument in arguments) startInfo.ArgumentList.Add(argument);

        using var process = new Process { StartInfo = startInfo };
        process.Start();
        var stdoutTask = process.StandardOutput.ReadToEndAsync();
        var stderrTask = process.StandardError.ReadToEndAsync();
        await process.WaitForExitAsync();
        return new CommandResult(process.ExitCode, await stdoutTask, await stderrTask);
    }

    private void SetBusy(bool busy)
    {
        _downloadButton.Enabled = !busy;
        _chooseAdbButton.Enabled = !busy;
        _refreshButton.Enabled = !busy;
        _installButton.Enabled = !busy;
        _launchButton.Enabled = !busy;
        Cursor = busy ? Cursors.WaitCursor : Cursors.Default;
    }

    private void SetStatus(string message, bool error = false)
    {
        _status.Text = message;
        _status.ForeColor = error ? Color.FromArgb(246, 145, 137) : Color.FromArgb(112, 225, 185);
    }

    private void Log(string message)
    {
        if (string.IsNullOrWhiteSpace(message)) return;
        if (InvokeRequired)
        {
            BeginInvoke(new Action(() => Log(message)));
            return;
        }
        _log.AppendText($"[{DateTime.Now:HH:mm:ss}] {message.Trim()}\r\n");
    }

    private sealed record AppSettings
    {
        public string? AdbPath { get; init; }
    }

    private sealed record CommandResult(int ExitCode, string Output, string Error);

    private sealed record DeviceEntry(string Id)
    {
        public override string ToString() => Id;
    }
}
