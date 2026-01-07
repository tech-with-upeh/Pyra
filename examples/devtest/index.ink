
/*
background: linear-gradient(45deg, transparent 49%, #a4a3c1 49% 51%, transparent 51%) , linear-gradient(-45deg, transparent 49%, #a4a3c1 49% 51%, transparent 51%);
        background-size: 3em 3em;
        background-color: #000000;
        opacity: 0.35
*/
        page("My APP") {
            @state num : 0
            view("mydiv", style={
                "height": "50px",
                "background-color": "green"
            }, onclick=(num) {
                num = num + 1
                print(num)
            }) {
                text('Click me and check console!')
            } 
        }
        