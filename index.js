const express = require('express');
const path = require('path');
const bodyParser = require('body-parser');
const cookieParser = require('cookie-parser');
const jwt = require('jsonwebtoken');
const crypto = require('crypto');
const { initDB, findUser, createUser } = require('./db');
const { deriveKey, encryptJSON } = require('./crypto-utils');
const secureUser = require('./secureUser');
require('dotenv').config();
const fs = require('fs');
const compression = require('compression');

const app = express();
const PORT = 3001;


const cors = require('cors');
const { fstat } = require('fs');

const multer = require('multer');
const sharp = require('sharp');
const { getData, saveData } = require('./secureUser');
const { getDB, findSSO } = require('./db');

const upload = multer({ limits: { fileSize: 1024 * 1024 * 1024 } }); // 5MB max

const corsOptions = {
  origin: function (origin, callback) {
    callback(null, true);
  },
  credentials: true // allow cookies
};

app.use(cors(corsOptions));

app.use(express.json({ limit: '1gb' }));  // or whatever size you need

app.use(compression());
app.use(cookieParser());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.static(path.join(__dirname, 'mc')));

app.post('/api/user/updatepfp', express.raw({ type: '*/*', limit: '1gb' }), async (req, res) => {
  try {
    const token = req.cookies.key;
    if (!token) return res.status(401).json({ error: 'Unauthorized' });

    if (!req.body || req.body.length === 0) {
      return res.status(400).json({ error: 'Empty image data' });
    }

    const pngBuffer = await sharp(req.body).png().toBuffer();
    const base64Image = pngBuffer.toString('base64');

    let userData = await getData(token);
    
    const newPlaintext = {
      ...userData.plaintext,
      pfp: { pfp: base64Image },
    };

    const newEncrypted = {
      ...userData.encrypted,
    };

    // Merge and save
    await saveData(token, { ...newPlaintext, ...newEncrypted });

    res.json({ success: true });
  } catch (err) {
    res.status(400).json({ error: 'Filetype not supported or image processing failed' });
    throw err;
  }
});

app.post('/api/sso', async (req, res) => {
  const { referrer, href } = req.body;
  try {
    // Check if the SSO entry already exists
    const existingSSO = await findSSO(referrer);
    if (!existingSSO) {
      // If it exists, update the location and trusted status
      return res.json({ error: 'SSO does not exist' });
    }

    res.json({ location: existingSSO.location, trusted: existingSSO.trusted, href });
  } catch (err) {
    console.error('Error saving SSO data:', err);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

app.get('/auth', async (req, res) => {
  res.sendFile(path.join(__dirname, 'pages', 'auth.html'));
});

app.get('/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'pages', 'login.html'));
});

app.get('/accounts', async (req, res) => {
  const cookie = req.cookies.offload;
  if (!cookie) return res.redirect('/login');

  let ref = req.params.referrer
  let href = req.params.href
  console.log(ref, href)
  let loginLink = ref&&href? ('/login?referrer='+ref+'&href='+href) : '/login';
  let accounts;
  try {
    accounts = JSON.parse(cookie); // offload cookie is a JSON object
    // if accounts is empty
    if (Object.keys(accounts).length === 0) return res.redirect(loginLink);
    if (accounts.error) return res.json(accounts);
  } catch (err) {
    return res.redirect(loginLink);
  }

  const db = getDB();
  let html = fs.readFileSync(path.join(__dirname, 'pages', 'accounts.html'), 'utf8');
  let renderedAccounts = '';

  for (const [username, password] of Object.entries(accounts)) {
    try {
      // Simulate GetUser with our JWT-less path: re-derive key and decrypt
      const [[user]] = await db.query('SELECT * FROM users WHERE username = ?', [username]);
      if (!user) continue;

      const key = require('./crypto-utils').deriveKey(password, Buffer.from(user.salt, 'base64'));
      const pfpBlob = typeof user.plaintext_blob === 'string' ? JSON.parse(user.plaintext_blob) : user.plaintext_blob;

      let pfp = pfpBlob.pfp;

      const imgSrc = pfp.pfp === null ? '/cache/Default.jpg' : `data:image/png;base64,${pfp.pfp}`;

      renderedAccounts += `
        <div class='account'>
          <div class='account-content'>
            <img src='${imgSrc}' alt='Avatar' class='avatar'/>
            <div>
              <span class='name'>${username}</span>
              <span class='email'>${pfpBlob.email}</span>
            </div>
          </div>
          <button class='remove-btn'>Remove</button>
        </div>`;
    } catch (e) {
      console.error(`Error processing user ${username}:`, e);
    }
  }

  html = html.replace('{accounts}', renderedAccounts);
  res.setHeader('Content-Type', 'text/html');
  res.send(html);
});

app.get('/profile/:username', async (req, res) => {
  const { username } = req.params;
  res.sendFile(path.join(__dirname, 'pages', 'profile.html'));
});


app.get('/cache/:name', async (req, res) => {
  const { name } = req.params;
  res.setHeader("Cache-Control", "public, max-age=3600");

  const token = req.cookies.key;
  const data = await secureUser.getData(token);
  if (name === 'navbar.js' && data.error === undefined) {
    let content = fs.readFileSync(path.join(__dirname, 'cache', name), 'utf8');
    content = content.replaceAll('{email}', data.all.email)
    content = content.replaceAll('{username}', data.username);
    content = content.replaceAll('\'{pfp}\'', JSON.stringify(data.all.pfp));
    content = content.replaceAll('"{displayname}"', JSON.stringify(data.all.displayName));

    // console.log(data.plaintext.pfp)

    res.contentType('text/javascript');
    res.send(content);
  } else {
    res.sendFile(path.join(__dirname, 'cache', name));
  }
});


app.get('/signup', (req, res) => {
  res.sendFile(path.join(__dirname, 'pages', 'signup.html'));
});

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'pages', 'index.html'));
});

// user["Created"].setRaw("time::now()");
// user["DisplayName"] = username;
// user["ShowStatus"].setRaw("true");
// user["SaveHistory"].setRaw("true");
// user["DataRetention"] = "7d";
// user["Pin"] = "";
// user["PinStatus"].setRaw("false");
// user["PinTime"] = "30";
// user["PinLastTimeUsed"] = "Never";
// user["Bio"] = "";
// user["LoginNotifs"].setRaw("true");
// user["GeneralNotifs"].setRaw("true");
// std::string logs = "[{\"time\":\""+getCurrentTimeISO8601()+"\",\"action\":\"Account Created\", \"ip\":\""+req.getIP()+"\"}]";

app.post('/api/signup', async (req, res) => {
  const { username, password } = req.body;

  // verify username and password
  // Username must be alphanumeric and between 3-20 characters
  const usernameRegex = /^[a-zA-Z0-9]{3,20}$/;
  if (!usernameRegex.test(username)) {
    return res.status(400).json({ error: 'Username must be alphanumeric and between 3-20 characters' });
  }
  // Password must be at least 8 characters long
  if (password.length < 8) {
    return res.status(400).json({ error: 'Password must be at least 8 characters long' });
  }
  const existing = await findUser(username);
  if (existing) return res.status(409).json({ error: 'Username exists' });

  const salt = crypto.randomBytes(16);
  const key = deriveKey(password, salt);
  const password_hash = crypto.createHash('sha256').update(password).digest('hex');

  const plaintext = { 
    email: `${username}@n11.dev`, 
    pfp: {pfp: null},
    created: new Date().toISOString(),
    displayName: username,
    bio: '',
    loginNotifs: true,
    generalNotifs: true,
    showStatus: true
  };
  const encrypted = {
    pin: '',
    pinStatus: false,
    pinTime: '30',
    pinLastTimeUsed: 'Never',
    saveHistory: true,
    dataRetention: '7d',
    logs: [{ time: new Date().toISOString(), action: 'Account Created', ip: req.headers['x-forwarded-for'] || req.ip }]
  };
  const encrypted_blob = encryptJSON(key, encrypted);
  const plaintext_blob = JSON.stringify(plaintext);
  const encrypted_keys = ['pin', 'pinStatus', 'pinTime', 'pinLastTimeUsed', 'saveHistory', 'dataRetention', 'logs'];

  await createUser(username, password_hash, salt.toString('base64'), encrypted_blob, plaintext_blob, encrypted_keys);
  const user = await findUser(username);
  if (!user) return res.status(500).json({ error: 'User creation failed' });
  const token = jwt.sign({ userId: user.uuid, key: key.toString('base64') }, process.env.JWT_SECRET);

  res.json({ success: true, token });
});

app.post('/api/login', async (req, res) => {
  const { username, password } = req.body;
  const user = await findUser(username);
  if (!user) return res.status(401).json({ error: 'Invalid credentials' });

  const hash = crypto.createHash('sha256').update(password).digest('hex');
  if (hash !== user.password_hash) return res.status(401).json({ error: 'Invalid credentials' });

  const key = deriveKey(password, Buffer.from(user.salt, 'base64'));
  const token = jwt.sign({ userId: user.uuid, key: key.toString('base64') }, process.env.JWT_SECRET);

  res.setHeader("set-cookie", `token=${token}`);
  res.json({ success: true, token });
});

app.get('/auth', async (req, res) => {
  res.sendFile(path.join(__dirname, 'pages', 'auth.html'));
});

app.get('/api/user/profile/:name', async (req, res) => {
  const { name } = req.params;
  const profile = await secureUser.findUser(name);
  if (!profile) return res.status(401).json({ error: 'User doesn\'t exist' });
  res.json({ [name]: profile.plaintext_blob });
});

app.get('/api/profile', async (req, res) => {
  try {
    const token = req.cookies.key;
    const data = await secureUser.getData(token);
    res.json({ data: data.all, encrypted_keys: await secureUser.getEncryptedKeys(token) });
  } catch (err) {
    res.status(401).json({ error: err.message });
  }
});

app.post('/api/profile', async (req, res) => {
  try {
    const token = req.cookies.key;
    await secureUser.saveData(token, req.body);
    res.json({ success: true });
  } catch (err) {
    res.status(401).json({ error: err.message });
  }
});

let accountSettings = async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.redirect('/login');
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    const html = fs.readFileSync(path.join(__dirname, 'pages', 'account', 'settings.html'), 'utf8');
    const renderedHtml = html
      .replace('{email}', data.all.email)
      .replace('{username}', data.username)
      .replace('\'{pfp}\'', JSON.stringify(data.plaintext.pfp))
      .replace('{displayname}', data.all.displayName)
      .replace('{bio}', data.all.bio || '')
      .replace('{generalNotifs}', data.all.generalNotifs ? 'checked' : '')
      .replace('{status}', data.all.showStatus ? 'checked' : '')
      .replace('{notifications}', data.all.loginNotifs ? 'checked' : '')
    res.setHeader('Content-Type', 'text/html');
    res.send(renderedHtml);
  } catch (err) {
    console.error('Error fetching account data:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
}

app.get('/account/', accountSettings)
app.get('/account/settings', accountSettings);


app.get('/account/privacy', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });

  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    const html = fs.readFileSync(path.join(__dirname, 'pages', 'account', 'privacy.html'), 'utf8');
    const renderedHtml = html
      .replace('{save}', data.all.saveHistory ? 'checked' : '')
      .replace('{retention}', data.all.dataRetention)
      .replace('{data}', JSON.stringify(data.encrypted.logs))

    res.setHeader('Content-Type', 'text/html');
    res.send(renderedHtml);
  } catch (err) {
    console.error('Error fetching privacy settings:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})

app.get('/account/security', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });

  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    const html = fs.readFileSync(path.join(__dirname, 'pages', 'account', 'security.html'), 'utf8');
    const renderedHtml = html
      .replace('{pinstatus}', data.encrypted.pinStatus ? 'checked' : '')
      .replace('{time}', data.encrypted.pinTime)
      .replace('{pinLastTimeUsed}', data.encrypted.pinLastTimeUsed)
      .replace('{pin}', data.encrypted.pin)
      .replace('{notifs}', data.plaintext.loginNotifs ? 'checked' : '')

    res.setHeader('Content-Type', 'text/html');
    res.send(renderedHtml);
  } catch (err) {
    console.error('Error fetching security settings:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})

app.post('/api/account/settings/update', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });

  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    // Update plaintext data
    const updates = req.body;
    
    const newPlaintext = {
      ...data.plaintext,
      displayName: updates.displayName || data.plaintext.displayName,
      bio: updates.bio || data.plaintext.bio,
      loginNotifs: updates.emailNotify === undefined ? data.plaintext.loginNotifs : updates.emailNotify,
      showStatus: updates.onlineStatus === undefined ? data.plaintext.showStatus : updates.onlineStatus
    };

    const newEncrypted = {
      ...data.encrypted,
    };

    // Merge and save
    await saveData(token, { ...newPlaintext, ...newEncrypted });
    res.json({ success: true });
  } catch (err) {
    console.error('Error updating account:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})


app.get('/account/delete', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });
    // return /account/delete.html
    res.sendFile(path.join(__dirname, 'pages', 'account', 'delete.html'));
  } catch (err) {
    console.error('Error fetching account data for deletion:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.get('/account/password', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    // Return the password hash
    res.sendFile(path.join(__dirname, 'pages', 'account', 'password.html'));
  } catch (err) {
    console.error('Error fetching password:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})

app.post('/api/account/verify', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    let user = await findUser(data.username);
    if (!user) return res.status(401).json({ error: 'User not found' });
    if (data.error) return res.status(401).json({ error: data.error });

    // Verify password
    const { password } = req.body;
    const hash = crypto.createHash('sha256').update(password).digest('hex');
    if (hash !== user.password_hash) {
      return res.status(401).json({ error: 'Invalid password' });
    }

    res.json({ success: true });
  } catch (err) {
    console.error('Error verifying password:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})

app.post('/api/account/password', async (req, res) => {
  // change password
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });
    const { current_password, new_password } = req.body;
    let user = await findUser(data.username);
    if (!user) return res.status(401).json({ error: 'User not found' });
    // Verify current password
    const hash = crypto.createHash('sha256').update(current_password).digest('hex');
    if (hash !== user.password_hash) {
      return res.status(401).json({ error: 'Invalid current password' });
    }
    // Update password
    const newSalt = crypto.randomBytes(16);
    const newKey = deriveKey(new_password, newSalt);
    const newPasswordHash = crypto.createHash('sha256').update(new_password).digest('hex');
    const encrypted_blob = encryptJSON(newKey, data.encrypted);
    const plaintext_blob = JSON.stringify(data.plaintext);
    const encrypted_keys = data.encrypted_keys;
    const db = getDB();
    await db.query(`UPDATE users SET password_hash = ?, salt = ?, encrypted_blob = ?, plaintext_blob = ?, encrypted_keys = ? WHERE uuid = ?`, [
      newPasswordHash,
      newSalt.toString('base64'),
      JSON.stringify(encrypted_blob),
      plaintext_blob,
      JSON.stringify(encrypted_keys),
      user.uuid
    ]);
    // Re-derive key and create new token
    const newToken = jwt.sign({ userId: user.uuid, key: newKey.toString('base64') }, process.env.JWT_SECRET);
    res.setHeader("set-cookie", `key=${newToken}; HttpOnly; Path=/; SameSite=Lax`);
    res.json({ success: true, token: newToken });
  } catch (err) {
    console.error('Error changing password:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
})

app.post('/api/account/delete', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });
    const user = await findUser(data.username);
    if (!user) return res.status(401).json({ error: 'User not found' });
    // Verify password
    const { password } = req.body;
    const hash = crypto.createHash('sha256').update(password).digest('hex');
    if (hash !== user.password_hash) {
      return res.status(401).json({ error: 'Invalid password' });
    }
    // Delete user
    const db = getDB();
    await db.query(`DELETE FROM users WHERE uuid = ?`, [user.uuid]);
    res.json({ success: true });
  } catch (err) {
    console.error('Error deleting account:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.post('/api/account/privacy/update', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    // Update encrypted data
    const updates = req.body;
    
    const newEncrypted = {
      ...data.encrypted,
      saveHistory: updates.saveActivity === undefined ? data.encrypted.saveHistory : updates.saveActivity,
      dataRetention: updates.retentionPeriod || data.encrypted.dataRetention,
      logs: data.encrypted.logs // Keep logs unchanged
    };

    // Merge and save
    await saveData(token, { ...data.plaintext, ...newEncrypted });
    res.json({ success: true });
  } catch (err) {
    console.error('Error updating privacy settings:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.post('/api/account/security/update', async (req, res) => {
  const token = req.cookies.key;
  if (!token) return res.status(401).json({ error: 'Unauthorized' });
  try {
    const data = await secureUser.getData(token);
    if (data.error) return res.status(401).json({ error: data.error });

    // Update encrypted data
    const updates = req.body;
    
    const newEncrypted = {
      ...data.encrypted,
      pinStatus: updates.pinEnabled === undefined ? data.encrypted.pinStatus : updates.pinEnabled,
      pin: updates.pinCode || data.encrypted.pin,
      pinTime: updates.pinTimeout || data.encrypted.pinTime,
      loginNotifs: updates.loginNotifications === undefined ? data.plaintext.loginNotifs : updates.loginNotifications
    };

    // Merge and save
    await saveData(token, { ...data.plaintext, ...newEncrypted });
    res.json({ success: true });
  } catch (err) {
    console.error('Error updating security settings:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// 404
app.use((req, res) => {
  res.status(404).sendFile(path.join(__dirname, 'pages', '404.html'));
});

(async () => {
  await initDB();
  app.listen(PORT, () => console.log(`Server running on http://localhost:${PORT}`));
})();