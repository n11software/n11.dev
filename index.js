// spin up a basic express website
let express = require('express')
let app = express()
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

app.listen(3000, () => {

})