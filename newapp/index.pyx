ht = "100px"

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

page("Home Page", cls="body") {
    @state txt : "predef"
    view("Nav", cls="nav", onclick=() {
        print("hello")
    }) {
        text(txt)
    }
    text("another txt")
}

page("about Page", route="/about") {
    view("nav") {
        text("My win", cls="mytx")
    }
}