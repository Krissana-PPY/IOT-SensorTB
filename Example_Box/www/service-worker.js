self.addEventListener('install', function(e) {
    e.waitUntil(
      caches.open('stock-checking').then(function(cache) {
        return cache.addAll([
          './',
          './index.html',
          './js/socket.io.min.js',
          './js/jquery.min.js',
          './fonts/PTSans-Regular.woff2'
        ]);
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
  