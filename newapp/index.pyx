ht = "100px"

@stylesheet & uni {
    body = {
        "height": ht,
        "width":"100px"
    } 
    btn = {
        "height":"100px",
        color: "red"
        } 
    media("min-width: 600px") { 
        btn {"height":"50px", color: "yellow"}
    }
}
page("HOME PAGE", cls="body", route="/abt") {
    vsr = 0
    @state num : 0
    @state mytext : "Click me and check console!"
    view("mydiv", style={
        "height": "50px",
        "background-color": "green"
    }, onclick=(num, mytext) {
        num = num + 1
        mytext = "Clicked!!"
        print(num)
    }) {
        text(mytext, style={
            "text-align": "center",
            "margin": "0px"
        })
    }
    view("countdiv", style={
        "margin-top": "20px",
        "font-size": "20px"
    }) {
        text('go to about', cls="btn", onclick=() {
            go("/")
        })
        
    } 
}
page("about") {
    inpage = "sdkd"
    text("This is the Ho.", cls="btn")
} 