page("HOME PAGE", style={
        "background-color": "black",
        "padding": "0px",
        "margin": "0px",
        "color": "white"
    }) {
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
            text('go to about', onclick=() {
                go("/about")
            })
            
        } 
  }


page("about") {
    text("This is the about page.")
}