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

nav = {
        "height":"80px",
        "backgroundColor":"green"
    }
div_id = 'nav'



window("my tetst page", style={
    "height":"100vh",
    "backgroundColor":"red"
}) {
    view("mydiv", style=nav) {
        img("img.png", style={
        "height":"200px",
        "width": "200px"
    })
    }
}
