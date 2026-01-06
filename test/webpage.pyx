
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

