import { writable } from 'svelte/store';

export const connected = writable(false);
export const vfoA = writable({ freq: 7074000, mode: 1 });
export const vfoB = writable({ freq: 7074000, mode: 1 });
export const meter = writable({ s: -120 });
export const spectrum = writable({ samples: new Uint8Array(800), width: 800 });
