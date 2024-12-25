export const CookieUtils = {
  // Maximum size for each cookie chunk (leaving room for cookie name and overhead)
  CHUNK_SIZE: 3800,

  // Store chunked data in cookies
  setCookieChunks(name, value) {
    // Clear any existing chunks
    this.clearCookieChunks(name);
    
    // Convert to string if needed
    const strValue = typeof value === 'object' ? JSON.stringify(value) : String(value);
    
    // Split into chunks
    const chunks = [];
    for (let i = 0; i < strValue.length; i += this.CHUNK_SIZE) {
      chunks.push(strValue.slice(i, i + this.CHUNK_SIZE));
    }
    
    // Store chunk count
    document.cookie = `${name}_count=${chunks.length};path=/`;
    
    // Store each chunk
    chunks.forEach((chunk, index) => {
      document.cookie = `${name}_${index}=${chunk};path=/`;
    });
  },

  // Read and reassemble chunked data from cookies
  getCookieChunks(name) {
    const cookies = document.cookie.split(';').reduce((acc, curr) => {
      const [key, value] = curr.trim().split('=');
      acc[key] = value;
      return acc;
    }, {});
    
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
  },

  // Clear all chunks for a given name
  clearCookieChunks(name) {
    const cookies = document.cookie.split(';');
    
    cookies.forEach(cookie => {
      const [key] = cookie.trim().split('=');
      if (key.startsWith(name + '_')) {
        document.cookie = `${key}=;path=/;expires=Thu, 01 Jan 1970 00:00:01 GMT`;
      }
    });
  }
};
