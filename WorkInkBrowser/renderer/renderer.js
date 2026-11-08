'use strict';

const addressInput = document.getElementById('address');
const addressForm = document.getElementById('address-form');
const backButton = document.getElementById('back');
const forwardButton = document.getElementById('forward');
const reloadButton = document.getElementById('reload');
const homeButton = document.getElementById('home');
const statusLine = document.getElementById('status');

function applyState(state) {
  if (!state) return;
  if (state.url && document.activeElement !== addressInput) {
    addressInput.value = state.url;
  }
  backButton.disabled = !state.canGoBack;
  forwardButton.disabled = !state.canGoForward;
  reloadButton.textContent = state.loading ? '×' : '↻';
  reloadButton.title = state.loading ? 'Остановить загрузку' : 'Обновить';
}

function showError(message) {
  statusLine.textContent = message || 'Не удалось выполнить действие.';
  statusLine.classList.add('visible');
}

function clearError() {
  statusLine.textContent = '';
  statusLine.classList.remove('visible');
}

addressForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  clearError();
  const result = await window.browser.navigate(addressInput.value);
  if (result && !result.ok) showError(result.error);
  addressInput.blur();
});

backButton.addEventListener('click', () => window.browser.back());
forwardButton.addEventListener('click', () => window.browser.forward());
reloadButton.addEventListener('click', () => window.browser.reload());
homeButton.addEventListener('click', () => {
  clearError();
  window.browser.home();
});

window.browser.onState(applyState);
window.browser.onError(showError);
window.browser.getState().then(applyState).catch((error) => showError(error.message));
