// spin up a basic express website
let express = require('express')
let app = express()
let fs = require('fs')
let crypto = require('crypto')
var cookieParser = require('cookie-parser')
app.use(cookieParser())
let compression = require('compression');

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
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const data = await response.json();
    return data
  } catch (error) {
    console.error('Error:', error.message);
  }
}


app.get('/link', (req, res) => {
  // res.header('Set-Cookie', 'cert='+fs.readFileSync('cert.pem').toString().replaceAll('\n', '\\n'))
  // res.send(req.cookies.key.replaceAll('\\n', '\n'))
  let key = req.cookies.key.replaceAll('\\n', '\n')
  let cert = req.cookies.cert.replaceAll('\\n', '\n')
  makeRequest('SELECT Code FROM Users WHERE Certificate=="'+cert+'"').then(d => {
    let bufferEncrypted = Buffer.from(d[0]["result"][0]["Code"], 'base64')
    let decrypted = crypto.privateDecrypt(key, bufferEncrypted)
    let file = fs.readFileSync(__dirname + '/pages/link.html')
    res.send(file.toString().replaceAll('{code}', decrypted.toString('utf-8')))
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

app.post('/api/link/new', (req, res) => {
  const characters = "01234556789";
  const randomString = generateRandomString(6, characters);
  let key = req.cookies.key.replaceAll('\\n', '\n')
  let cert = req.cookies.cert.replaceAll('\\n', '\n')
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