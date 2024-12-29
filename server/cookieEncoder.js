const CookieUtils = {
  // Maximum size for each cookie chunk (matching client-side)
  CHUNK_SIZE: 3800,

  setCookieChunks(name, value) {
    const cookies = [];
    
    // Convert to string if needed
    const strValue = typeof value === 'object' ? JSON.stringify(value) : String(value);
    
    // Split into chunks
    const chunks = [];
    for (let i = 0; i < strValue.length; i += this.CHUNK_SIZE) {
      chunks.push(strValue.slice(i, i + this.CHUNK_SIZE));
    }
    
    // Create Set-Cookie strings
    let tenyrs = 10 * 365 * 24 * 60 * 60;
    cookies.push(`${name}_count=${chunks.length}; Max-Age=${tenyrs}; SameSite=None; Secure; path=/; `);
    chunks.forEach((chunk, index) => {
      cookies.push(`${name}_${index}=${chunk}; Max-Age=${tenyrs}; SameSite=None; Secure; path=/; `);
    });
    
    return cookies;
  },

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
