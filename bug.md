# Bug Tracking - Windows Port

## Bug 1: Audio distorto (RISOLTO)

**Sintomo**: Audio completamente distorto su Windows, sembrava problema di sample rate.

**Root cause**: In `src/portaudio.c`, la latenza suggerita era impostata a `0.0`, causando buffer troppo piccoli per il resampling.

**Fix applicato** (linea 573):
```c
// Prima:
outputParameters.suggestedLatency = 0.0;

// Dopo:
outputParameters.suggestedLatency = Pa_GetDeviceInfo(padev)->defaultLowOutputLatency;
```

**Status**: RISOLTO

---

## Bug 2: MIDI non funziona dopo selezione dispositivo (IN CORSO)

**Sintomo**: Il dispositivo MIDI viene riconosciuto e appare nel menu. Quando si mette la spunta per attivarlo, non succede nulla di visibile. Non si sa se:
1. Il dispositivo viene aperto correttamente
2. I messaggi MIDI arrivano
3. I messaggi vengono processati

**File coinvolti**:
- `Windows/windows_midi.c` - Layer 1 (hardware Windows)
- `src/midi2.c` - Layer 2 (traduzione MIDI -> azioni)
- `src/midi3.c` - Layer 3 (esecuzione azioni)
- `src/midi_menu.c` - Menu di configurazione

**Debug aggiunto**:
In `Windows/windows_midi.c`:
- `register_midi_device()`: aggiunto debug con fflush per vedere se viene chiamato e se midiInOpen/midiInStart hanno successo
- `MidiInProc()` callback: aggiunto debug per vedere se i messaggi MIDI arrivano

**Prossimi passi**:
1. Ricompilare e testare
2. Guardare il log quando si seleziona il dispositivo MIDI
3. Se il dispositivo si apre correttamente, muovere un controllo MIDI e vedere se appaiono messaggi "MIDI IN: ..."
4. Se i messaggi arrivano ma non succede nulla, il problema è nella configurazione (manca midi.props o MidiCommandsTable vuota)
5. Se i messaggi NON arrivano, il problema è nel callback Windows

**Note**:
- Il sistema MIDI di piHPSDR richiede un file di configurazione per mappare i controlli MIDI alle azioni
- Verificare se esiste un file `midi.props` nella directory di configurazione

**Status**: IN CORSO - in attesa di test con debug aggiunto
