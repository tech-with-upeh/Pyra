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
} */

page("MAIN") {
    onmount() {
        ctx = draw('hero')
        ctx.setFill("#850000ff")
        ctx.rect(10, 10, 70 ,150)

        for(i=0 : i < 50 : i++) {
            print(12+to_int("34"))
        }
    }
    canvas("hero", height=500, width=500)
}
