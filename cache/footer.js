let ssF = document.createElement("link")
ssF.rel = "stylesheet"
ssF.href = "/cache/footer.css"
document.getElementsByTagName("head")[0].appendChild(ssF)

let Links = {
  "Account": {
    "Manage": "//account.n11.dev/",
    "Sign Up": "//n11.dev/signup",
    "Login": "//n11.dev/login",
    "Mail": "//mail.n11.dev/"
  },
  "Entertainment": {
    "Kaos": "//kaos.n11.dev/",
    "Box": "//box.n11.dev/"
  },
  "Developer": {
    "API": "//developer.n11.dev/docs/api",
    "Software": "//developer.n11.dev/docs/apps",
  },
  "Enterprise": {
    "Manage": "//enterprise.n11.dev/manage",
    "Quote": "//enterprise.n11.dev/quote",
    "Help": "//enterprise.n11.dev/help",
    "Pricing": "//enterprise.n11.dev/pricing",
    "Contact": "//enterprise.n11.dev/contact"
  },
  "Store": {
    "Phone": "//store.n11.dev/phone"
  },
  "Support" : {
    "1-877-999-NULL": "tel:877999NULL",
    "support@n11.dev": "mailto:support@n11.dev"
  }
}

let createFooter = () => {
  let footer = document.createElement('div')
  footer.classList.add('footer')
  let row1 = document.createElement('div')
  row1.classList.add('row')
  Object.entries(Links).forEach(type => {
    let col = document.createElement('div')
    col.classList.add('col')
    let title = document.createElement('span')
    title.textContent = type[0]
    title.classList.add('title')
    let sectionName = type[0]
    col.appendChild(title)
    Object.entries(type[1]).forEach(link => {
      let linkA = document.createElement('a')
      linkA.href = Links[sectionName][link[0]]
      linkA.textContent = link[0]
      col.appendChild(linkA)
    })
    row1.appendChild(col)
  })
  footer.appendChild(row1)

  // END
  let end = document.createElement('div')
  end.classList.add('row', 'end')
  let cpyrt = document.createElement('span')
  cpyrt.classList.add('copyright')
  cpyrt.textContent = 'Copyright © 2024 N11 LLC. All rights reserved.'
  end.appendChild(cpyrt)
  let site = document.createElement('div')
  let sitemap = document.createElement('a')
  sitemap.href = '//n11.dev/sitemap'
  sitemap.textContent = 'Sitemap'
  site.appendChild(sitemap)
  let privacyPolicy = document.createElement('a')
  privacyPolicy.href = '//n11.dev/privacy'
  privacyPolicy.textContent = 'Privacy Policy'
  site.appendChild(privacyPolicy)
  let tos = document.createElement('a')
  tos.href = '//n11.dev/tos'
  tos.textContent = 'Terms of Service'
  site.appendChild(tos)

  end.appendChild(site)
  footer.appendChild(end)


  document.body.appendChild(footer)
}

createFooter()

let isMobile = window.innerWidth <= 1000

window.onresize = () => isMobile = window.innerWidth <= 1000

let toggleFooterMenu = e => {
  if (!isMobile) return
  let title = e.target
  let parent = e.target.parentNode
  let links = parent.querySelectorAll('a')
  links.forEach(link => {
    link.style.display = link.style.display == "block" ? "none" : "block"
  })
}

document.querySelectorAll('div.footer>div.row>div.col>span.title').forEach(e => {
  e.onclick = toggleFooterMenu
})