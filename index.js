// spin up a basic express website
let express = require('express')
let app = express()
let fs = require('fs')
let crypto = require('crypto')
var cookieParser = require('cookie-parser')
app.use(cookieParser())
let compression = require('compression');
let EncLib = require('./EncLib')
const bodyParser = require('body-parser')
const multer = require('multer');
const sharp = require('sharp');
const upload = multer({ storage: multer.memoryStorage() });
const CookieUtils = require('./server/cookieEncoder.js');

app.use(bodyParser.json());

// setting up the static files directory
app.use(express.static('public'))
app.use(compression())

// define a route to serve the HTML file
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/pages/index.html')
})


app.get('/login', (req, res) => {
  res.sendFile(__dirname + '/pages/login.html')
})

app.get('/signup', (req, res) => {
  res.sendFile(__dirname + '/pages/signup.html')
})

app.get('/cache/:name', async (req, res) => {
  res.setHeader('Cache-Control', 'public, max-age=3600')
    let data, un, email
    if (req.params.name.includes('navbar.js')) {
    try {
      const username = req.cookies.username;
      if (!username) {
        data = { error: "Not logged in" };
      }

      const result = await makeRequest(`
        SELECT * FROM Users 
        WHERE Username == '${username.toLowerCase()}'
      `);

      if (result && result[0].result && result[0].result[0]) {
        data = { pfp: result[0].result[0].ProfilePicture };
        un = result[0].result[0].Username;
        email = un + '@n11.dev';
      } else {
        data = { "error": "Not logged in" };
      }
    } catch (error) {
      console.error('Error:', error);
      data = { error: "Failed to fetch profile picture" };
    }
    let file = fs.readFileSync(__dirname + '/cache/navbar.js', 'utf8');
    file = file.replaceAll('\'{pfp}\'', JSON.stringify(data))
    file = file.replaceAll('\'{username}\'', JSON.stringify(un))
    file = file.replaceAll('\'{email}\'', JSON.stringify(email))
    res.setHeader('Content-Type', 'application/javascript');
    res.send(file);
  } else
    res.sendFile(
      `cache/${req.params.name}`,
      { root: __dirname }
    )
})

async function makeRequest(body) {
  try {
    const response = await fetch('http://localhost:8000/sql', {
      method: 'POST',
      headers: {
        'Authorization': 'Basic ' + Buffer.from('root:root').toString('base64'),
        'Accept': 'application/json',
        'surreal-ns': 'Development',
        'surreal-db': 'N11'
      },
      'body': body
    });

    if (!response.ok) {
      console.log(body)
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const data = await response.json();
    return data
  } catch (error) {
    console.error('Error:', error.message);
  }
}


app.get('/link', (req, res) => {
  const cert = CookieUtils.getCookieChunks(req.cookies, 'cert');
  const key = CookieUtils.getCookieChunks(req.cookies, 'key');
  const signature = CookieUtils.getCookieChunks(req.cookies, 'signature');

  if (!cert || !key || !signature) {
    return res.json({"error": "No authentication data provided"});
  }

  let authData = EncLib.DecodeAuth({
    Certificate: cert,
    Key: key,
    Signature: signature
  });
  
  if (!authData) {
    return res.json({"error": "Invalid authentication data"});
  }

  if (!EncLib.VerifyPair(authData.cert, authData.signature)) {
    return res.json({"error": "Invalid certificate signature"});
  }

  // Continue with existing logic...
  makeRequest('SELECT * FROM Users WHERE Username == "'+req.cookies.username+'"').then(e => {
    if (e[0]["result"].length <= 0) {
      res.json({"error": "No user found with this certificate"});
    } else {
      let user = e[0]["result"][0];
      let file = fs.readFileSync(__dirname + '/pages/link.html', 'utf8');
      file = file.replace('{code}', user["Code"])
      res.send(file);
    }
    return
  }).catch(e => {
    console.error('Error:', e);
    res.json({"error": "An error occurred"});
  })
  // res.send('something went wrong')
})

function generateRandomString(length, characters) {
  let result = "";
  for (let i = 0; i < length; i++) {
    const randomIndex = Math.floor(Math.random() * characters.length);
    result += characters.charAt(randomIndex);
  }
  return result;
}

app.post('/api/user/create', async (req, res) => {
  let username = req.cookies.username;
  const usernameRegex = /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+$/;
  if (!usernameRegex.test(username)) return res.json({"error": "Invalid username."});
  // to lower username
  username = username.toLowerCase();

  // Create a new key and cert
  try {
    let { publicKey, privateKey, signature } = await EncLib.CreateSignedPair();
    let verified = EncLib.VerifyPair(publicKey, signature);
    if (!verified) {
      res.json({"error": "Could not verify the signature"});
    }
    const characters = "01234556789";
    const randomString = generateRandomString(6, characters);
    let code = await EncLib.Encrypt(randomString, publicKey);
    makeRequest(`
    $exists = ((SELECT * FROM Users WHERE Username == '${username}')==[]);

    IF $exists = true THEN
        INSERT INTO Users {
          Certificate: '${publicKey}',
          Code: '${code}',
          Created: time::now(),
          Username: '${username}',
          ProfilePicture: null
        };
    END;`).then(async e => {
      if (e[1]["result"] == null) {
        // The username is already taken, notify the user.
        res.json({"error": "This username is taken"});
      } else {
        // Original plan was to store the data in cookies but it is too large so we will use localStorage
        let redirURL = await EncLib.Encrypt("/profiles/" + e[1]["result"][0]["id"].split('Users:')[1], publicKey);
        let Cert = CookieUtils.setCookieChunks('cert', Buffer.from(publicKey).toString('base64'))
        let Key = CookieUtils.setCookieChunks('key', Buffer.from(privateKey).toString('base64'))
        let Signature = CookieUtils.setCookieChunks('signature', Buffer.from(signature).toString('base64'))
        let AllCookies = Cert.concat(Key).concat(Signature)
        res.setHeader('Set-Cookie', AllCookies)
        res.json({
          "Certificate": Buffer.from(publicKey).toString('base64'),
          "Key": Buffer.from(privateKey).toString('base64'),
          "Signature": Buffer.from(signature).toString('base64'),
          "Redirect": redirURL
        });
      }
    });
  } catch (err) {
    console.error('Error:', err);
    res.json({"error": "An error occurred"});
    // TODO: Log all errors to the DB and make a dashboard to view everything
  }
});

app.post('/api/link/new', (req, res) => {
  // Check if cookies exist and have auth data
  if (!req.cookies || !req.cookies.auth) {
    return res.json({"error": "No authentication data provided"});
  }
  
  let authData = EncLib.DecodeAuth(req.cookies.auth);
  if (!authData) {
    return res.json({"error": "Invalid authentication data"});
  }

  const characters = "01234556789";
  const randomString = generateRandomString(6, characters);
  
  let { cert, key } = authData;
  const bufferMessage = Buffer.from(randomString, 'utf-8');
  const encrypted = crypto.publicEncrypt(cert, bufferMessage);
  let newCode = encrypted.toString('base64');
  
  makeRequest('UPDATE Users SET Code = "'+newCode+'" WHERE Certificate = "'+cert+'"')
  res.json({"Code": randomString})
})

app.get('/api/link/scan', (req, res) => {
  // get client ip
  let ip = req.headers['x-forwarded-for'] || req.connection.remoteAddress;

  makeRequest('select LoggedIn from Linker where id == Linker:⟨'+ip+'⟩').then(e => {
    let users = e[0]["result"][0]['LoggedIn']
    let s = {}
    // loop through users
    Object.entries(users).forEach(user => {
      // append user key to s with the value as the value.substring(1)
      s[user[0]] = parseInt(user[1][0])
    })

    res.send(s)
  })
})

app.get('/', (req, res) => {
  res.sendFile(__dirname + '/pages/index.html')
})

app.get('/profile', (req, res) => {
    res.sendFile(__dirname + '/pages/account/profile.html');
});

app.post('/api/user/updatepfp', upload.single('pfp'), async (req, res) => {
  // TODO: Add authentication
    try {
        if (!req.file) {
            return res.json({ error: "No file uploaded" });
        }

        // Convert image to PNG and resize
        const processedImage = await sharp(req.file.buffer)
            .resize(200, 200, { // Set your desired dimensions
                fit: 'cover',
                position: 'center'
            })
            .png()
            .toBuffer();

        // Convert to base64 for database storage
        const base64Image = processedImage.toString('base64');

        // Update the database
        const username = req.cookies.username;
        if (!username) {
            return res.json({ error: "Not logged in" });
        }

        const result = await makeRequest(`
            UPDATE Users 
            SET ProfilePicture = '${base64Image}'
            WHERE Username == '${username.toLowerCase()}'
        `);

        res.json({ success: true });
    } catch (error) {
        console.error('Error:', error);
        res.json({ error: "Failed to process image" });
    }
});

app.get('/api/user/getpfp', async (req, res) => {
    try {
      const username = req.cookies.username;
      if (!username) {
        return res.json({ error: "Not logged in" });
      }

      const result = await makeRequest(`
        SELECT ProfilePicture FROM Users 
        WHERE Username == '${username.toLowerCase()}'
      `);

      if (result && result[0].result && result[0].result[0]) {
        res.json({ pfp: result[0].result[0].ProfilePicture });
      } else {
        res.json({ "error": "Not logged in" });
      }
    } catch (error) {
      console.error('Error:', error);
      res.json({ error: "Failed to fetch profile picture" });
    }
});

app.listen(3000, () => {

})