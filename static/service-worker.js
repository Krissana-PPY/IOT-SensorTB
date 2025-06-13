self.addEventListener('install', function(e) {
  e.waitUntil(
    caches.open('stock-checking').then(function(cache) {
      return cache.addAll([
        '/',
        '/static/js/socket.io.min.js',
        '/static/js/jquery-3.6.0.min.js'
      ]).catch(function(err) {
        console.warn('ServiceWorker cache addAll error:', err);
      });
    })
  );
});
