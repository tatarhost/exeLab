'use strict';

const path = require('node:path');
const {
  app,
  BrowserWindow,
  WebContentsView,
  ipcMain,
  dialog,
  session
} = require('electron');

const START_URL = 'https://work.ink/';
const TOOLBAR_HEIGHT = 76;
const browserViews = new Map();
const openWindows = new Set();

function isWebUrl(value) {
  try {
    const parsed = new URL(value);
    return parsed.protocol === 'https:' || parsed.protocol === 'http:';
  } catch {
    return false;
  }
}

function normalizeAddress(value) {
  const input = String(value || '').trim();
  if (!input) return START_URL;
  const candidate = /^[a-z][a-z\d+.-]*:\/\//i.test(input)
    ? input
    : `https://${input}`;
  const parsed = new URL(candidate);
  if (parsed.protocol !== 'https:' && parsed.protocol !== 'http:') {
    throw new Error('Only HTTP and HTTPS addresses are supported.');
  }
  return parsed.toString();
}

function stateFor(view) {
  const contents = view.webContents;
  const history = contents.navigationHistory;
  return {
    url: contents.getURL() || START_URL,
    title: contents.getTitle() || 'WorkInk Browser',
    canGoBack: history.canGoBack(),
    canGoForward: history.canGoForward(),
    loading: contents.isLoading()
  };
}

function notifyState(win, view) {
  if (win.isDestroyed() || win.webContents.isDestroyed() || view.webContents.isDestroyed()) return;
  const state = stateFor(view);
  win.webContents.send('browser:state', state);
  win.setTitle(state.title + ' — WorkInk Browser');
}

function resizeView(win, view) {
  if (win.isDestroyed() || view.webContents.isDestroyed()) return;
  const bounds = win.getContentBounds();
  view.setBounds({
    x: 0,
    y: TOOLBAR_HEIGHT,
    width: Math.max(0, bounds.width),
    height: Math.max(0, bounds.height - TOOLBAR_HEIGHT)
  });
}

function createBrowserWindow(initialUrl = START_URL) {
  const win = new BrowserWindow({
    width: 1280,
    height: 860,
    minWidth: 720,
    minHeight: 520,
    title: 'WorkInk Browser',
    backgroundColor: '#ffffff',
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      webSecurity: true,
      allowRunningInsecureContent: false,
      webviewTag: false
    }
  });

  const view = new WebContentsView({
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      webSecurity: true,
      allowRunningInsecureContent: false,
      webviewTag: false
    }
  });

  browserViews.set(win, view);
  openWindows.add(win);
  win.contentView.addChildView(view);
  resizeView(win, view);
  win.on('resize', () => resizeView(win, view));

  view.webContents.setWindowOpenHandler(({ url }) => {
    if (isWebUrl(url)) createBrowserWindow(url);
    return { action: 'deny' };
  });

  view.webContents.on('will-navigate', (event, url) => {
    if (!isWebUrl(url)) event.preventDefault();
  });

  for (const eventName of [
    'did-navigate',
    'did-navigate-in-page',
    'did-start-loading',
    'did-stop-loading',
    'page-title-updated'
  ]) {
    view.webContents.on(eventName, () => notifyState(win, view));
  }

  view.webContents.on('did-fail-load', (_event, code, description, url, isMainFrame) => {
    if (isMainFrame && code !== -3 && !win.isDestroyed()) {
      win.webContents.send('browser:error', `Не удалось открыть страницу: ${description} (${code})`);
    }
  });

  win.on('closed', () => {
    openWindows.delete(win);
    browserViews.delete(win);
  });

  win.loadFile(path.join(__dirname, 'renderer', 'index.html'));
  view.webContents.loadURL(normalizeAddress(initialUrl)).catch((error) => {
    if (!win.isDestroyed()) win.webContents.send('browser:error', error.message);
  });

  return win;
}

function getViewForSender(sender) {
  const win = BrowserWindow.fromWebContents(sender);
  return win ? browserViews.get(win) : undefined;
}

ipcMain.handle('browser:get-state', (event) => {
  const view = getViewForSender(event.sender);
  return view ? stateFor(view) : null;
});

ipcMain.handle('browser:navigate', async (event, address) => {
  const view = getViewForSender(event.sender);
  if (!view) return { ok: false, error: 'Browser window is unavailable.' };
  try {
    await view.webContents.loadURL(normalizeAddress(address));
    return { ok: true };
  } catch (error) {
    return { ok: false, error: error.message };
  }
});

ipcMain.handle('browser:back', (event) => {
  const view = getViewForSender(event.sender);
  if (view && view.webContents.navigationHistory.canGoBack()) {
    view.webContents.navigationHistory.goBack();
  }
});

ipcMain.handle('browser:forward', (event) => {
  const view = getViewForSender(event.sender);
  if (view && view.webContents.navigationHistory.canGoForward()) {
    view.webContents.navigationHistory.goForward();
  }
});

ipcMain.handle('browser:reload', (event) => {
  const view = getViewForSender(event.sender);
  if (view) view.webContents.reload();
});

ipcMain.handle('browser:home', (event) => {
  const view = getViewForSender(event.sender);
  if (view) return view.webContents.loadURL(START_URL);
});

app.whenReady().then(() => {
  createBrowserWindow();

  app.on('activate', () => {
    if (openWindows.size === 0) createBrowserWindow();
  });

  session.defaultSession.on('will-download', async (event, item, contents) => {
    item.pause();
    try {
      const owner = BrowserWindow.fromWebContents(contents);
      const result = await dialog.showSaveDialog(owner || undefined, {
        title: 'Сохранить файл',
        defaultPath: path.join(app.getPath('downloads'), item.getFilename())
      });
      if (result.canceled || !result.filePath) {
        item.cancel();
        return;
      }
      item.setSavePath(result.filePath);
      item.resume();
    } catch {
      item.cancel();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
