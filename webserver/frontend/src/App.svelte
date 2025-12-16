<script>
  let connected = false;
  let frequency = 7074000;
</script>

<main>
  <h1>piHPSDR Web</h1>
  <p>Status: {connected ? 'Connected' : 'Disconnected'}</p>
  <p>Frequency: {(frequency / 1e6).toFixed(6)} MHz</p>
</main>

<style>
  main {
    padding: 20px;
    max-width: 1200px;
    margin: 0 auto;
  }
  h1 {
    color: #4CAF50;
    margin-bottom: 20px;
  }
</style>
