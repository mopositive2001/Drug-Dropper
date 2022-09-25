/*
 *  Used to connect the front end to the backend and 3DS NFC Reader
 */

const ws = new WebSocket("ws://10.20.10.137:10101"); // 3DS Websocket Server
const db = new WebSocket("ws://localhost:1337");     // Localhost Server

let amounts = {
    "red": 10,
    "green": 10,
    "yellow": 10,
    "blue": 10
};

db.onmessage = function(msg) {
    console.log(msg.data);
    const data = JSON.parse(msg.data);
    switch(data.pid) {
        case 0:
            let string_pres='Dispensing <br>';
            for(var i=0; i<data.prescriptions.length; i++){
                let pdata = data.prescriptions[i];
                
                let oldnum = amounts[pdata.medicine.toLowerCase()];
                console.log("Oldnum = ", oldnum);
                let newnum = oldnum - parseInt(pdata.dosage);
                console.log("NewNum = ", newnum);
                amounts[pdata.medicine.toLowerCase()] = newnum;

                let classname = pdata.medicine;
                classname = classname.charAt(0).toUpperCase() + classname.slice(1);

                document.getElementById(pdata.medicine).innerHTML = `${classname} (${newnum})`;
                string_pres=string_pres+classname+' '+oldnum+'<br>';
            }
            document.getElementById("touch-pad").innerHTML=string_pres; 
            document.getElementById("medicine").style.animation="medicine_display 5s";
            document.getElementById("medicine").style.display="none";
            break;
        case 2:
            document.getElementById("touch-pad").style.fontSize='10px';
            document.getElementById("touch-pad").innerHTML=`Hello ${data.fullname}, please enter you pin number`; 
            break;
    }
};

ws.onmessage = function(msg) {
    let data = JSON.parse(msg.data);

    if(!data)
        return;

    if(data.cardid == -1)
        return;

    const pincode = parseInt(prompt('Please enter your pin number here: '));
    console.log("Got pin code: ", pincode);
    
    db.send(JSON.stringify({ pid: 0, encUUID: data.cardid, pin: pincode }));
    db.send(JSON.stringify({ pid: 2, encUUID: data.cardid, pin: pincode }));
};

setInterval(function() {
    ws.send("IWANTYOURCODE"); // Send nfc request code
}, 500);
