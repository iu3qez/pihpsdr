/**
 * Audio player using Web Audio API with adaptive buffering.
 */
export class AudioPlayer {
  constructor(sampleRate = 48000) {
    this.sampleRate = sampleRate;
    this.ctx = null;
    this.bufferQueue = [];
    this.isPlaying = false;
    this.nextPlayTime = 0;
    this.bufferDuration = 0.1; // 100ms buffer
    this.minBufferSize = 3;    // Minimum buffers before starting
  }

  async init() {
    if (this.ctx) return;

    this.ctx = new (window.AudioContext || window.webkitAudioContext)({
      sampleRate: this.sampleRate
    });

    // Resume context (required by browsers)
    if (this.ctx.state === 'suspended') {
      await this.ctx.resume();
    }

    console.log('AudioContext initialized, sample rate:', this.ctx.sampleRate);
  }

  /**
   * Add audio samples to the queue.
   * @param {Int16Array} samples - Signed 16-bit PCM samples
   */
  addSamples(samples) {
    if (!this.ctx) return;

    // Convert Int16 to Float32
    const floatSamples = new Float32Array(samples.length);
    for (let i = 0; i < samples.length; i++) {
      floatSamples[i] = samples[i] / 32768;
    }

    // Create audio buffer
    const buffer = this.ctx.createBuffer(1, floatSamples.length, this.sampleRate);
    buffer.getChannelData(0).set(floatSamples);

    this.bufferQueue.push(buffer);

    // Start playback if we have enough buffers
    if (!this.isPlaying && this.bufferQueue.length >= this.minBufferSize) {
      this._startPlayback();
    }
  }

  _startPlayback() {
    if (this.isPlaying) return;

    this.isPlaying = true;
    this.nextPlayTime = this.ctx.currentTime + this.bufferDuration;
    this._scheduleBuffers();
  }

  _scheduleBuffers() {
    while (this.bufferQueue.length > 0) {
      const buffer = this.bufferQueue.shift();

      const source = this.ctx.createBufferSource();
      source.buffer = buffer;
      source.connect(this.ctx.destination);

      // Schedule playback
      const playTime = Math.max(this.nextPlayTime, this.ctx.currentTime);
      source.start(playTime);

      this.nextPlayTime = playTime + buffer.duration;
    }

    // Check for underrun
    if (this.bufferQueue.length === 0) {
      this.isPlaying = false;
    }
  }

  setVolume(value) {
    // For future: add gain node
  }

  stop() {
    if (this.ctx) {
      this.ctx.close();
      this.ctx = null;
    }
    this.bufferQueue = [];
    this.isPlaying = false;
  }
}

export const audioPlayer = new AudioPlayer();
