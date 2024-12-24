import { MlKem1024 } from "https://esm.sh/mlkem";

function base64ToBytes(base64) {
  if (!base64) throw new Error("No input provided");
  
  if (base64 instanceof Uint8Array) {
    return base64;
  }

  if (typeof base64 !== 'string') {
    base64 = String(base64);
  }

  // Clean and normalize base64 string
  base64 = base64.trim()
    .replace(/[\r\n\t]/g, '')
    .replace(/\s+/g, '')
    .replace(/[^A-Za-z0-9+/=]/g, ''); // Remove invalid chars

  // Split into chunks to handle long strings
  const chunkSize = 1024;
  const chunks = [];
  
  for (let i = 0; i < base64.length; i += chunkSize) {
    const chunk = base64.slice(i, i + chunkSize);
    try {
      const binary = atob(chunk);
      const bytes = new Uint8Array(binary.length);
      for (let j = 0; j < binary.length; j++) {
        bytes[j] = binary.charCodeAt(j);
      }
      chunks.push(bytes);
    } catch (e) {
      console.error("Chunk decode error:", e);
      throw new Error("Invalid base64 format");
    }
  }

  // Combine chunks
  const totalLength = chunks.reduce((acc, chunk) => acc + chunk.length, 0);
  const result = new Uint8Array(totalLength);
  let offset = 0;
  for (const chunk of chunks) {
    result.set(chunk, offset);
    offset += chunk.length;
  }
  
  return result;
}

export async function Encrypt(message, cert) {
  try {
    let sender = new MlKem1024();
    let certBytes = typeof cert === 'string' ? base64ToBytes(cert) : cert;
    let [ciphertext, sharedSecret] = await sender.encap(certBytes);
    
    // Derive key using SHA-256
    let keyMaterial = await crypto.subtle.digest('SHA-256', sharedSecret);
    let key = await crypto.subtle.importKey(
      'raw',
      keyMaterial,
      { name: 'AES-GCM' },
      false,
      ['encrypt']
    );

    let iv = crypto.getRandomValues(new Uint8Array(12));
    let encoded = new TextEncoder().encode(message);
    
    let encrypted = await crypto.subtle.encrypt(
      { name: 'AES-GCM', iv },
      key,
      encoded
    );

    // Split encrypted data into ciphertext and auth tag
    let encryptedArray = new Uint8Array(encrypted);
    let tag = encryptedArray.slice(-16); // Last 16 bytes are the auth tag
    let encryptedData = encryptedArray.slice(0, -16);

    return JSON.stringify({
      encapsulated: btoa(String.fromCharCode(...ciphertext)),
      iv: btoa(String.fromCharCode(...iv)),
      tag: btoa(String.fromCharCode(...tag)),
      data: btoa(String.fromCharCode(...encryptedData))
    });
  } catch (err) {
    console.error('Encryption error:', err);
    throw new Error('Encryption failed');
  }
}

function parseRedirect(data) {
  const parsed = JSON.parse(data);
  return JSON.stringify({
    encapsulated: parsed.encapsulated,
    iv: parsed.iv,
    tag: parsed.tag,
    data: parsed.data
  });
}

export async function Decrypt(code, privKey) {
  if (typeof code === 'string' && code.startsWith('{')) {
    // Already JSON string
    code = code;
  } else {
    // Needs parsing
    code = parseRedirect(code);
  }
  
  try {
    let { encapsulated, iv, tag, data } = JSON.parse(code);
    let recipient = new MlKem1024();
    
    let privateKeyBytes = typeof privKey === 'string' ? base64ToBytes(privKey) : privKey;
    let encapsulatedBytes = base64ToBytes(encapsulated);
    let ivBytes = base64ToBytes(iv);
    let tagBytes = base64ToBytes(tag);
    let dataBytes = base64ToBytes(data);
    
    let sharedSecret = await recipient.decap(encapsulatedBytes, privateKeyBytes);
    
    // Derive key using SHA-256
    let keyMaterial = await crypto.subtle.digest('SHA-256', sharedSecret);
    let key = await crypto.subtle.importKey(
      'raw',
      keyMaterial,
      { name: 'AES-GCM' },
      false,
      ['decrypt']
    );

    // Combine data and tag for decryption
    let ciphertextWithTag = new Uint8Array(dataBytes.length + tagBytes.length);
    ciphertextWithTag.set(dataBytes);
    ciphertextWithTag.set(tagBytes, dataBytes.length);

    let decrypted = await crypto.subtle.decrypt(
      { name: 'AES-GCM', iv: ivBytes },
      key,
      ciphertextWithTag
    );

    return new TextDecoder().decode(decrypted);
  } catch (err) {
    console.error('Decryption error:', err);
    throw new Error('Decryption failed');
  }
}

window.EncLib = { Encrypt, Decrypt, parseRedirect };