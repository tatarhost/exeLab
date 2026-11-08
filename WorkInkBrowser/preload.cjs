'use strict';

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('browser', {
  getState: () => ipcRenderer.invoke('browser:get-state'),
  navigate: (address) => ipcRenderer.invoke('browser:navigate', address),
  back: () => ipcRenderer.invoke('browser:back'),
  forward: () => ipcRenderer.invoke('browser:forward'),
  reload: () => ipcRenderer.invoke('browser:reload'),
  home: () => ipcRenderer.invoke('browser:home'),
  onState(callback) {
    if (typeof callback !== 'function') return () => {};
    const listener = (_event, state) => callback(state);
    ipcRenderer.on('browser:state', listener);
    return () => ipcRenderer.removeListener('browser:state', listener);
  },
  onError(callback) {
    if (typeof callback !== 'function') return () => {};
    const listener = (_event, message) => callback(message);
    ipcRenderer.on('browser:error', listener);
    return () => ipcRenderer.removeListener('browser:error', listener);
  }
});
