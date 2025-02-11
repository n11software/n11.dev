// Load CSS
let ss = document.createElement("link")
ss.rel = "stylesheet"
ss.href = "/cache/navbar.css"
document.getElementsByTagName("head")[0].appendChild(ss)

// SVG paths
let lines = `<path fill-rule="evenodd" d="M3 6.75A.75.75 0 0 1 3.75 6h16.5a.75.75 0 0 1 0 1.5H3.75A.75.75 0 0 1 3 6.75ZM3 12a.75.75 0 0 1 .75-.75h16.5a.75.75 0 0 1 0 1.5H3.75A.75.75 0 0 1 3 12Zm0 5.25a.75.75 0 0 1 .75-.75h16.5a.75.75 0 0 1 0 1.5H3.75a.75.75 0 0 1-.75-.75Z" clip-rule="evenodd" />`
let x = `<path fill-rule="evenodd" d="M5.47 5.47a.75.75 0 0 1 1.06 0L12 10.94l5.47-5.47a.75.75 0 1 1 1.06 1.06L13.06 12l5.47 5.47a.75.75 0 1 1-1.06 1.06L12 13.06l-5.47 5.47a.75.75 0 0 1-1.06-1.06L10.94 12 5.47 6.53a.75.75 0 0 1 0-1.06Z" clip-rule="evenodd" />`

// pfp
let hasPFP = false
let pfp = document.createElement('img')

// Create the navbar
let navLinks = {"Enterprise": "//enterprise.n11.dev", "Developers": "//developers.n11.dev", 
  "Mail": "//mail.n11.dev", "Social": "//social.n11.dev", "Store": "//store.n11.dev"}

try {
  if (nav === undefined) {
    navLinks = {"Enterprise": "//enterprise.n11.dev", "Developers": "//developers.n11.dev", 
      "Mail": "//mail.n11.dev", "Social": "//social.n11.dev", "Store": "//store.n11.dev"}
  } else {
    navLinks = nav
  }
} catch (e) {
  
}

let createMobileNav = () => {
  let mobileNav = document.createElement('div')
  mobileNav.classList.add('navbar-mobile')
  {
    for (let key in navLinks) {
      let link = document.createElement('a')
      link.href = navLinks[key]
      link.textContent = key
      mobileNav.appendChild(link)
    }
    if (!hasPFP) {
      mobileNav.appendChild(document.createElement('a'))
      mobileNav.lastElementChild.textContent = 'Login'
      mobileNav.lastElementChild.classList.add('login')
      mobileNav.lastElementChild.setAttribute('href', '//n11.dev/login')
    }
  }
  document.body.insertBefore(mobileNav, document.currentScript);
}

// Add this outside any function to track popup state
let popupVal = false;

let togglePopUp = (val) => {
  popupVal = val;
  if (val) {
    document.querySelector('.popup').style.display = 'flex'
  } else {
    document.querySelector('.popup').style.display = 'none'
  }
}

let createNavbar = () => {
  let navbar = document.createElement('div')
  navbar.classList.add('navbar')

  let links = document.createElement('div')

  let logo = document.createElement('a')
  logo.href = '//n11.dev'
  logo.classList.add('no-hover')
  let logoImg = document.createElement('img')
  logoImg.src = '/cache/logo.png'
  logoImg.classList.add('logo')
  logo.appendChild(logoImg)
  links.appendChild(logo)

  {
    for (let key in navLinks) {
      let link = document.createElement('a')
      link.href = navLinks[key]
      link.textContent = key
      links.appendChild(link)
    }
  }
  navbar.appendChild(links)

  let login = document.createElement('div')
  { // Login/Account button
    console.log(hasPFP)
    if (hasPFP)  {
      {
        // create pop up
        let popUp = document.createElement('div')
        popUp.classList.add('popup')
        popUp.style.display = 'none'

        // Add close button for mobile
        let closeBtn = document.createElement('button')
        closeBtn.classList.add('close-button')
        closeBtn.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor">'+x+'</svg>'
        closeBtn.onclick = () => {
          togglePopUp(false)
          val = false
        }
        popUp.appendChild(closeBtn)

        let top = document.createElement('div')
        
        // Create hidden file input
        let fileInput = document.createElement('input')
        fileInput.type = 'file'
        fileInput.accept = 'image/*'
        fileInput.style.display = 'none'
        fileInput.onchange = (e) => {
          const file = e.target.files[0];
          if (file) {
            uploadPfp(file);
          }
        }
        top.appendChild(fileInput)
        
        let pfpDupe = pfp.cloneNode(true)
        pfpDupe.onclick = () => fileInput.click()
        pfpDupe.style.cursor = 'pointer'
        top.appendChild(pfpDupe)
        
        let username = document.createElement('span')
        username.classList.add('username')
        username.textContent = '{username}'
        top.appendChild(username)
        let email = document.createElement('span')
        email.classList.add('email')
        email.textContent = '{email}'
        top.appendChild(email)
        let bottom = document.createElement('div')
        let link = document.createElement('a')
        link.classList.add('link')
        link.textContent = 'Switch Account'
        let url = window.location.href
        link.href = '/accounts?redir=' + url
        bottom.appendChild(link)
        let manage = document.createElement('a')
        manage.classList.add('manage')
        manage.textContent = 'Manage Account'
        manage.href = '/account'
        bottom.appendChild(manage)
        popUp.appendChild(top)
        popUp.appendChild(bottom)
        document.body.appendChild(popUp)
      }
      let pfpDupe = pfp.cloneNode(true)
      let val = false
      pfpDupe.onclick = () => {togglePopUp(!val);val = !val}
      document.body.onclick = e => {
        const popup = document.querySelector('.popup');
        // Check if click target is inside popup
        // get classlist of e.target
        let classList = e.target.classList
        if (!popup.contains(e.target) && e.target !== pfpDupe && !e.target.classList.contains('mobilepfp') && val) {
          e.preventDefault()
          togglePopUp(false);
          val = false;
        }
      }
      // togglePopUp(!val);val = !val
      console.log(pfpDupe)
      login.appendChild(pfpDupe)
      navbar.appendChild(login)
    } else {
      let loginButton = document.createElement('a')
      loginButton.setAttribute('href', '//n11.dev/login')
      loginButton.textContent = 'Login'
      loginButton.classList.add('login')
      login.appendChild(loginButton)
      navbar.appendChild(login)
    }
  }

  let mobile = document.createElement('div')
  mobile.classList.add('mobile')
  {
    let logo = document.createElement('a')
    logo.style.height = '64px'
    logo.setAttribute('href', '//n11.dev/')
    let logoImg = document.createElement('img')
    logoImg.src = '/cache/logo.png'
    logoImg.classList.add('logo')
    let span = document.createElement('span')
    span.textContent = 'N11'
    logo.appendChild(logoImg)
    logo.appendChild(span)
    mobile.appendChild(logo)
  }

  let mobileOpt = document.createElement('div')
  mobileOpt.classList.add('mbopt')
  let mobilePfp = pfp.cloneNode(true)
  mobilePfp.onclick = () => {
    togglePopUp(!popupVal)
  }
  mobilePfp.classList.add('mobilepfp')
  mobileOpt.appendChild(mobilePfp)
  let menuSvg = document.createElement('svg')
  menuSvg.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" class="menu" id="mobile-open">'+lines+'</svg>'
  mobileOpt.appendChild(menuSvg)
  mobile.appendChild(mobileOpt)

  navbar.appendChild(mobile)

  document.body.prepend(navbar)
  createMobileNav()
}

// Add this function after createNavbar but before using it
const uploadPfp = async (file) => {
  if (!file) return;
  
  try {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.readAsArrayBuffer(file);
      
      reader.onload = () => {
        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/api/user/updatepfp', true);
        xhr.setRequestHeader('Content-Type', file.type);
        
        xhr.onload = function() {
          if (xhr.status === 200) {
            const data = JSON.parse(xhr.responseText);
            if (data.error) {
              alert(data.error);
            } else {
              document.querySelectorAll('.pfp, .large-pfp').forEach(img => {
                img.src = URL.createObjectURL(file);
              });
            }
            resolve(data);
          } else {
            reject(new Error('Upload failed'));
          }
        };
        
        xhr.onerror = () => reject(new Error('Network error'));
        xhr.send(reader.result);
      };
      
      reader.onerror = reject;
    });
  } catch (error) {
    console.error('Error uploading profile picture:', error);
  }
}

// Manage the navbar
let navline

let data = '{pfp}'

let createNavLine = () => {
  let navLine = document.createElement('div')
  navLine.style.width = '0'
  navLine.style.height = '3px'
  navLine.style.backgroundColor = 'white'
  navLine.style.position = 'absolute'
  navLine.style.transition = 'all 0.25s ease'
  document.body.appendChild(navLine)
  navline = navLine
}

if (data.error != undefined) {
  hasPFP = false
} else if (data.pfp !== undefined) {
  pfp.src = `data:image/png;base64,${data.pfp}`
  if (data.pfp == null) {
    pfp.src = `/Default.jpg`;
  }
  pfp.classList.add('pfp')
  hasPFP = true
}
createNavbar()
let navbar = document.querySelector('.navbar-mobile')
navbar.style.top = '69px'
let mobileMenuOpen = document.querySelector('#mobile-open')
mobileMenuOpen.addEventListener('click', () => {
  if (!s) {
    //open
    document.querySelector('.navbar').style.position = 'fixed'
    navbar.style.display = 'flex'
    mobileMenuOpen.innerHTML = x
    disableScroll()
    let t = 0
    let addFade = (e) => {
      let i = .15
      document.querySelectorAll(e).forEach(el => {
        el.style.animation = 'fade-in .5s 1'
        el.style.animationDelay = t +'s'
        el.style.animationFillMode = 'forwards'
        i < e.length ? t+=.15 : i++
      })
    }
    
    addFade('.navbar-mobile > a:not(.login)')
    s = true
  } else {
    //close
    document.querySelector('.navbar').style.position = 'inherit'
    navbar.style.display = 'none'
    enableScroll()
    mobileMenuOpen.innerHTML = lines
    navline.style.opacity = '0'
    s = false
  }
})

let s = false // closed

// left: 37, up: 38, right: 39, down: 40,
// spacebar: 32, pageup: 33, pagedown: 34, end: 35, home: 36
var keys = {37: 1, 38: 1, 39: 1, 40: 1};

function preventDefault(e) {
  e.preventDefault();
}

function preventDefaultForScrollKeys(e) {
  if (keys[e.keyCode]) {
    preventDefault(e);
    return false;
  }
}

// modern Chrome requires { passive: false } when adding event
var supportsPassive = false;
try {
  window.addEventListener("test", null, Object.defineProperty({}, 'passive', {
    get: function () { supportsPassive = true; } 
  }));
} catch(e) {}

var wheelOpt = supportsPassive ? { passive: false } : false;
var wheelEvent = 'onwheel' in document.createElement('div') ? 'wheel' : 'mousewheel';

// call this to Disable
function disableScroll() {
  window.addEventListener('DOMMouseScroll', preventDefault, false); // older FF
  window.addEventListener(wheelEvent, preventDefault, wheelOpt); // modern desktop
  window.addEventListener('touchmove', preventDefault, wheelOpt); // mobile
  window.addEventListener('keydown', preventDefaultForScrollKeys, false);
}

// call this to Enable
function enableScroll() {
  window.removeEventListener('DOMMouseScroll', preventDefault, false);
  window.removeEventListener(wheelEvent, preventDefault, wheelOpt); 
  window.removeEventListener('touchmove', preventDefault, wheelOpt);
  window.removeEventListener('keydown', preventDefaultForScrollKeys, false);
}


document.addEventListener('DOMContentLoaded', () => {
  createNavLine()

  let menuItems = document.querySelectorAll('.navbar > :first-child > a:not(.no-hover)')
  menuItems.forEach((menuItem) => {
    menuItem.addEventListener('mouseenter', () => {
      navline.style.opacity = '1'
      navline.style.left = menuItem.offsetLeft + 'px'
      navline.style.top = menuItem.offsetTop + menuItem.offsetHeight + 'px'
      navline.style.width = menuItem.offsetWidth + 'px'
    })

    menuItem.addEventListener('mouseleave', () => {
      navline.style.opacity = '0'
    })
  })

  let mobileItem = document.querySelectorAll('.navbar-mobile > a:not(.login')
  mobileItem.forEach((mobileItem) => {
    mobileItem.addEventListener('mouseenter', () => {
      navline.style.opacity = '1'
      navline.style.left = mobileItem.offsetLeft + 'px'
      navline.style.top = mobileItem.offsetTop + document.querySelector('.navbar').offsetHeight + mobileItem.offsetHeight + 'px'
      navline.style.width = mobileItem.offsetWidth + 'px'
    })

    mobileItem.addEventListener('mouseleave', () => {
      navline.style.opacity = '0'
    })
  })
})

// Logs
console.log(`
                                                     0000 
                     0000000000000000              0000000
                 000000000000000000000000        000000000
              000000000000000000000000000000   000000000  
           0000000000000          00000000000000000000    
          0000000000                   0000000000000      
        000000000                        0000000000       
       00000000                        0000000000000      
      0000000                         00000000000000      
     0000000                        000000000 00000000    
    0000000                       000000000    0000000    
   0000000                      000000000       0000000   
   000000                     000000000          000000   
   000000                    00000000            0000000  
  0000000                 000000000               000000  
  0000000                00000000                 000000  
   000000              000000000                 0000000  
   000000            000000000                   000000   
   0000000         000000000                     000000   
    000000       000000000                      0000000   
    0000000    000000000                       0000000    
     0000000 000000000                        0000000     
      00000000000000                        00000000      
       000000000000                       000000000       
        00000000000                     000000000         
      0000000000000000              000000000000          
    00000000000000000000000000000000000000000             
  000000000     000000000000000000000000000               
000000000          00000000000000000000                   
0000000                    00000                          
 0000                                                     

Developer job applications at https://n11.dev/jobs/.
If you're not a developer and you see this, we recommend that you avoid using the developer console, as you can be social engineered to allow a scammer to access your account.`)