// Fetch wrapper with logging for all requests
window.fetch = (function(origFetch) {
  return function(...args) {
    const [resource, config] = args;
    const method = (config && config.method) || 'GET';
    const body = (config && config.body) || null;
    console.log(`[FETCH] ${method} ${resource}`);
    if (body) console.log(`[FETCH BODY]`, body);
    const start = Date.now();
    return origFetch.apply(this, args).then(response => {
      const duration = Date.now() - start;
      console.log(`[FETCH RESPONSE] ${method} ${resource} (${duration}ms)`, response);
      return response;
    }).catch(err => {
      console.error(`[FETCH ERROR] ${method} ${resource}`, err);
      throw err;
    });
  };
})(window.fetch);
