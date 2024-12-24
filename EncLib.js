const { MlKem1024 } = require('crystals-kyber-js');
const crypto = require('crypto');
const fs = require('fs');

let CreateCA = () => {
  let { publicKey, privateKey } = crypto.generateKeyPairSync('rsa', {
    modulusLength: 2048
  });
  fs.writeFileSync('CACert.pem', publicKey.export({ type: 'spki', format: 'pem' }));
  fs.writeFileSync('CAKey.pem', privateKey.export({ type: 'pkcs8', format: 'pem' }));
};

let CreateSignedPair = async () => {
  let kyber = new MlKem1024();
  let [publicKey, privateKey] = await kyber.generateKeyPair();

  let caPrivateKey = fs.readFileSync('CAKey.pem', 'utf8');
  let sign = crypto.createSign('SHA256');
  sign.update(publicKey);
  sign.end();
  let signature = sign.sign(caPrivateKey);

  return { publicKey, privateKey, signature };
};

let VerifyPair = (cert, signature) => {
  let caPublicKey = fs.readFileSync('CACert.pem', 'utf8');
  let verify = crypto.createVerify('SHA256');
  verify.update(cert);
  verify.end();
  return verify.verify(caPublicKey, signature);
};

let Encrypt = async (message, cert) => {
  let sender = new MlKem1024();
  let [ciphertext, sharedSecret] = await sender.encap(cert);
  // console.log('Shared Secret:', sharedSecret);
  let key = crypto.createHash('sha256').update(sharedSecret).digest();
  let iv = crypto.randomBytes(12);
  let cipher = crypto.createCipheriv('aes-256-gcm', key, iv);
  let encrypted = Buffer.concat([cipher.update(message, 'utf8'), cipher.final()]);
  let authTag = cipher.getAuthTag();
  let result = JSON.stringify({
    encapsulated: Buffer.from(ciphertext).toString('base64'),
    iv: iv.toString('base64'),
    tag: authTag.toString('base64'),
    data: encrypted.toString('base64')
  });
  // console.log('Encrypted Result:', result);
  return result;
};

let Decrypt = async (code, privKey) => {
  let { encapsulated, iv, tag, data } = JSON.parse(code);
  // console.log('Decrypt Input:', { encapsulated, iv, tag, data });
  let recipient = new MlKem1024();
  let sharedSecret = await recipient.decap(Buffer.from(encapsulated, 'base64'), privKey);
  // console.log('Shared Secret:', sharedSecret);
  let key = crypto.createHash('sha256').update(sharedSecret).digest();
  let decipher = crypto.createDecipheriv('aes-256-gcm', key, Buffer.from(iv, 'base64'));
  decipher.setAuthTag(Buffer.from(tag, 'base64'));
  let decrypted = Buffer.concat([
    decipher.update(Buffer.from(data, 'base64')),
    decipher.final()
  ]);
  // console.log('Decrypted Result:', decrypted.toString('utf8'));
  return decrypted.toString('utf8');
};

let DecodeAuth = (data) => {
  if (!data) {
    console.error('DecodeAuth: No data provided');
    return null;
  }
  
  try {
    // Handle both string and object input
    const auth = typeof data === 'string' ? JSON.parse(data) : data;
    
    if (!auth.Certificate || !auth.Key || !auth.Signature) {
      console.error('DecodeAuth: Missing required fields');
      return null;
    }

    // Directly return the raw buffer data without converting to string
    return {
      cert: Buffer.from(auth.Certificate, 'base64'),  // Remove toString()
      key: Buffer.from(auth.Key, 'base64'),          // Remove toString()
      signature: Buffer.from(auth.Signature, 'base64')  // Remove toString()
    };
  } catch (err) {
    console.error('Auth decode error:', err);
    return null;
  }
};

module.exports = {
  CreateSignedPair,
  VerifyPair,
  Encrypt,
  Decrypt,
  DecodeAuth // Add new export
};

if (require.main === module) {
  let readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
  });
  readline.question('\x1b[31mThis will destroy the current database. Are you sure you want to continue? (y/N) ', (answer) => {
    if (answer.toLowerCase() === 'y') {
      CreateCA();
    }
    readline.close();
  });
}