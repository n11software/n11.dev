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

app.get('/cache/:name', (req, res) => {
  res.setHeader('Cache-Control', 'public, max-age=31536000')
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
  res.sendFile(__dirname + '/pages/link.html')
})

app.post('/api/link', (req, res) => {
  // Check if the cert is valid.
  if (!req.body) {
    return res.json({"error": "No authentication data provided"});
  }

  let authData = EncLib.DecodeAuth(req.body);
  if (!authData) {
    return res.json({"error": "Invalid authentication data"});
  }

  let { cert, key, signature } = authData;
  if (!cert || !key || !signature) {
    return res.json({"error": "Missing authentication components"});
  }

  if (!EncLib.VerifyPair(cert, signature)) {
    return res.json({"error": "Invalid certificate signature"});
  }

  // Continue with existing logic...
  makeRequest('SELECT * FROM Users WHERE Username == "'+req.cookies.username+'"').then(e => {
    console.log(req.cookies.username)
    console.log(e)
    if (e[0]["result"] == []) {
      res.json({"error": "No user found with this certificate"});
    } else {
      let user = e[0]["result"][0];
      res.json({
        "Code": user["Code"]
      });
    }
  })
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
            Username: '${username}'
        };
    END;`).then(async e => {
      if (e[1]["result"] == null) {
        // The username is already taken, notify the user.
        res.json({"error": "This username is taken"});
      } else {
        // Original plan was to store the data in cookies but it is too large so we will use localStorage
        let redirURL = await EncLib.Encrypt("/profiles/" + e[1]["result"][0]["id"].split('Users:')[1], publicKey);
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

app.listen(3000, () => {

})