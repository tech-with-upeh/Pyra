

    page("My APP") {
        @state mytxt : "My state text"
        view("mydiv", style={
            "height": "50px",
            "background-color": "green"
        }, onclick=(mytxt) {
            mytxt = "my changed text"
            print(mytxt)
        }) {
            text(mytxt)
        } 
    }
    
    