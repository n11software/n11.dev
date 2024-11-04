let addFadeIn = a => {
  let t = 0
  const observer = new IntersectionObserver(entries => {
    // Loop over the entries
    let i = .25
    entries.forEach(entry => {
      // If the element is visible
      if (entry.isIntersecting) {
        // Add the animation class
        entry.target.style.animation = 'fade-in .5s 1'
        entry.target.style.animationDelay = t +'s'
        entry.target.style.animationFillMode = 'forwards'
        i < entries.length ? t+=.25 : i++
      }
    })
  })

  document.querySelectorAll(a).forEach(e => {
    observer.observe(e)
  })
}