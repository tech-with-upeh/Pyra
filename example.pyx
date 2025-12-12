o = true

kkk = 90
x= 12
if(x < 5) {
    x = x + 1
}
th = 122
function po() {
    pass
    wa = 1222
    op =12
}
 we =  09 
 po(we, 12, "yeyy")

if true {
    p = 12 
}

pls =12 +233 * 1223
print(pls)


emcc web/generated.cpp -o web/main.js -sEXPORTED_FUNCTIONS='["_main","_invokeVNodeCallback","_invokeEventHandler","_js_insertHTML","_js_setTitle","_malloc","_free","_js_updateElement","_js_removeElement"]' -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","stringToUTF8","lengthBytesUTF8"]' -sALLOW_MEMORY_GROWTH=1 -sASSERTIONS=1 -O2
emcc main.cpp -o web/main.js -sEXPORTED_FUNCTIONS=_main,_invokeVNodeCallback,_invokeEventHandler,_js_insertHTML,_js_updateElement,_js_removeElement,_js_setTitle,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,stringToUTF8,lengthBytesUTF8 -sALLOW_MEMORY_GROWTH=1 -sASSERTIONS=1 -O2