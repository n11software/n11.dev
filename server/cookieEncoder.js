const CookieUtils = {
  getCookieChunks(cookies, name) {
    const chunkCount = parseInt(cookies[`${name}_count`]);
    if (!chunkCount) return null;
    
    let result = '';
    for (let i = 0; i < chunkCount; i++) {
      const chunk = cookies[`${name}_${i}`];
      if (!chunk) return null;
      result += chunk;
    }
    
    try {
      return JSON.parse(result);
    } catch {
      return result;
    }
  }
};

module.exports = CookieUtils;
