const crypto = require('crypto');
const fs = require('fs');

// Generate RSA key pair in memory
let { publicKey, privateKey } = crypto.generateKeyPairSync('rsa', {
  modulusLength: 2048,
  publicKeyEncoding: {
    type: 'spki',
    format: 'pem',
  },
  privateKeyEncoding: {
    type: 'pkcs8',
    format: 'pem',
  },
});

console.log('Generated Public Key:\n', publicKey);
console.log('Generated Private Key:\n', privateKey);

// Function to save keys to files
function saveKeysToFile() {
  fs.writeFileSync('key.pem', privateKey, { encoding: 'utf8' });
  fs.writeFileSync('cert.pem', publicKey, { encoding: 'utf8' });
  console.log('Keys saved to key.pem and cert.pem');
}

// Function to load a key or cert from a string
function loadKeyOrCert(keyString) {
  // Verify that the input is a valid PEM key or cert
  if (keyString.startsWith('-----BEGIN') && keyString.endsWith('-----END PRIVATE KEY-----\n') ||
      keyString.endsWith('-----END PUBLIC KEY-----\n')) {
    return keyString;
  } else {
    throw new Error('Invalid key or certificate string');
  }
}

// Function to encrypt a message
function encryptMessage(message, publicKey) {
  const bufferMessage = Buffer.from(message, 'utf-8');
  const encrypted = crypto.publicEncrypt(publicKey, bufferMessage);
  return encrypted.toString('base64');
}

// Function to decrypt a message
function decryptMessage(encryptedMessage, privateKey) {
  const bufferEncrypted = Buffer.from(encryptedMessage, 'base64');
  const decrypted = crypto.privateDecrypt(privateKey, bufferEncrypted);
  return decrypted.toString('utf-8');
}

// Save keys to files

// Example usage of loadKeyOrCert
const loadedPublicKey = loadKeyOrCert(fs.readFileSync('cert.pem').toString());
const loadedPrivateKey = loadKeyOrCert(fs.readFileSync('key.pem').toString());

const message = 'hello';
console.log('Original Message:', message);

// const encryptedMessage = encryptMessage(message, loadedPublicKey);
// console.log('Encrypted Message:', encryptedMessage);

const decryptedMessage = decryptMessage(message, loadedPrivateKey);
console.log('Decrypted Message:', decryptedMessage);
