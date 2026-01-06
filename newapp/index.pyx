/* ht = "100px"

@stylesheet & uni {
    body = {
        "height": "100vh",
        "width":"100vw",
        "background-color": "dark-grey"
    } 
    nav = {
        "height":"0px",
        "background-color": "red",
        overflow: "hidden"
        } 
    tx = {
        color: "light-grey",
        margin: "0px"
    }
    primarysec = {
         color: "dark-grey",
        margin: "0px"
    }
    media("min-width: 600px") { 
        nav {"height":"100px", color: "yellow"}
    }
}

def pr(a, b) {
    return "as" + "kf"
}

page("Home Page", cls="body") {
    @state txt : "predef"
    sh = to_int("we")
    onmount() {
        print(txt)
        print("Mounted")
    }
    view("Nav", cls="nav", onclick=() {
        print("hello")
    }) {
        text(txt)
    }
    text("another txt")
    canvas("mycanvas", height=400, width=400)
}

page("about Page", route="/about") {
    view("nav") {
        text("My win", cls="mytx")
    }
}
} */

page(route="about") {
    h = Platform().height
    w = Platform().width
    onmount() {
        ctx = draw('hero')

        ctx.setStroke('rgba(255, 255, 255, 0.3)')
        ctx.lineWidth(1)
        ctx.line(50,50, 55, 55)   
    }
    animatefps() {
        print("resized")
    }
    listener(mousemove) {
        print("kk")
    }
    canvas("hero", height=h, width=w)
}
