self.addEventListener('install', function(e) {
  e.waitUntil(
    caches.open('stock-checking').then(function(cache) {
      return cache.addAll([
        '/',
        '/index.html',
        '/js/socket.io.min.js',
        '/js/jquery-3.6.0.min.js'
      ]).catch(function(err) {
        console.warn('ServiceWorker cache addAll error:', err);
      });
    })
  );
});

self.addEventListener('fetch', function(e) {
  e.respondWith(
    caches.match(e.request).then(function(response) {
      return response || fetch(e.request);
    })
  );
});
