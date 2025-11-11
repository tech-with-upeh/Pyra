/*
body = {
    "height":"100vh",
    "backgroundColor":"red"
}
nav = {
        "height":"80px",
        "backgroundColor":"green"
    }
txt = {
        "height":"80px",
        "backgroundColor":"blue"
    }
title= "My Pyx page"
window(title, style=body) {
    mydivid = "navbar"
    view(mydivid, style=nav) {
        text("view\np1") {
        }
        txt3 = "my  third text"
        text(txt3)
        print("my page title is "+ title)
    }
    txtc = "text oustide div"
    text(txtc, style=txt)
}
*/
navh = "80px"

div_id = 'nav'


app() {
    page("my tetst page",id="kkk", cls="llii", style={
        "height":"100vh",
        "backgroundColor":"red"
    }) {
        view("mydiv",  cls="uioo", style={
        "height": navh,
        "backgroundColor":"green"
    }, onclick=() {
            print("hello")
        }) {
            img("img.png", style={
            "height":"200px",
            "width": "200px"
        })
        }
    }
}


/*
app() {
    page() {
        view
        text
    }

    page() {
        view
        text
    }
} */