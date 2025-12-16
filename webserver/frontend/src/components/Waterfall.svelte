<script>
  import { onMount, onDestroy } from 'svelte';
  import { spectrum } from '../lib/stores.js';

  export let width = 800;
  export let height = 300;
  export let waterfallHeight = 200;

  let canvas;
  let ctx;
  let waterfallCanvas;
  let waterfallCtx;
  let waterfallImageData;
  let animationId;

  // Color gradient for spectrum (blue -> green -> yellow -> red)
  const gradient = [];
  for (let i = 0; i < 256; i++) {
    if (i < 64) {
      gradient.push([0, 0, i * 4]);
    } else if (i < 128) {
      gradient.push([0, (i - 64) * 4, 255 - (i - 64) * 4]);
    } else if (i < 192) {
      gradient.push([(i - 128) * 4, 255, 0]);
    } else {
      gradient.push([255, 255 - (i - 192) * 4, 0]);
    }
  }

  onMount(() => {
    ctx = canvas.getContext('2d');
    waterfallCtx = waterfallCanvas.getContext('2d');
    waterfallImageData = waterfallCtx.createImageData(width, waterfallHeight);

    // Subscribe to spectrum updates
    const unsubscribe = spectrum.subscribe(draw);

    return () => {
      unsubscribe();
      if (animationId) cancelAnimationFrame(animationId);
    };
  });

  function draw(data) {
    if (!ctx || !data.samples) return;

    const { samples } = data;
    const spectrumHeight = height - waterfallHeight;

    // Clear spectrum area
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, spectrumHeight);

    // Draw spectrum line
    ctx.strokeStyle = '#4CAF50';
    ctx.lineWidth = 1;
    ctx.beginPath();

    for (let x = 0; x < width && x < samples.length; x++) {
      const dbm = samples[x];
      const y = spectrumHeight - (dbm / 255) * spectrumHeight;
      if (x === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.stroke();

    // Fill under curve
    ctx.lineTo(width, spectrumHeight);
    ctx.lineTo(0, spectrumHeight);
    ctx.closePath();
    ctx.fillStyle = 'rgba(76, 175, 80, 0.3)';
    ctx.fill();

    // Update waterfall
    updateWaterfall(samples);
  }

  function updateWaterfall(samples) {
    // Scroll waterfall down
    const imgData = waterfallImageData.data;
    const rowSize = width * 4;

    // Move existing data down one row
    for (let y = waterfallHeight - 1; y > 0; y--) {
      const destOffset = y * rowSize;
      const srcOffset = (y - 1) * rowSize;
      for (let i = 0; i < rowSize; i++) {
        imgData[destOffset + i] = imgData[srcOffset + i];
      }
    }

    // Draw new row at top
    for (let x = 0; x < width && x < samples.length; x++) {
      const dbm = samples[x];
      const [r, g, b] = gradient[dbm] || [0, 0, 0];
      const offset = x * 4;
      imgData[offset] = r;
      imgData[offset + 1] = g;
      imgData[offset + 2] = b;
      imgData[offset + 3] = 255;
    }

    waterfallCtx.putImageData(waterfallImageData, 0, 0);
  }

  function handleClick(event) {
    // Calculate frequency from click position
    const rect = canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const ratio = x / width;
    // Dispatch event for frequency change
    canvas.dispatchEvent(new CustomEvent('freqclick', {
      detail: { ratio },
      bubbles: true
    }));
  }
</script>

<div class="waterfall-container">
  <canvas
    bind:this={canvas}
    {width}
    height={height - waterfallHeight}
    on:click={handleClick}
  />
  <canvas
    bind:this={waterfallCanvas}
    {width}
    height={waterfallHeight}
    on:click={handleClick}
  />
</div>

<style>
  .waterfall-container {
    display: flex;
    flex-direction: column;
    background: #1a1a1a;
    border: 1px solid #333;
    border-radius: 4px;
    overflow: hidden;
  }
  canvas {
    display: block;
    cursor: crosshair;
  }
</style>
