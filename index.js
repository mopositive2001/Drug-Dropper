/*
 * Entire backend, connects to Postgresql backend retrieves client information.
 */

const { Client } = require('pg');
const WebSocketServer = require('ws').Server;
const wss = new WebSocketServer({port: 1337});

const crypto = require('node:crypto');

const client =  new Client({
    host: '10.20.6.38',
    port: 5432,
    user: 'vanle',
    password: 'password',
    database: 'patientdb'
});

client.connect(function(err) {
    if(err) return;
    console.log("Connected to db :)");  
});

const SECRET_ADMIN_KEY = 38520743894573283; // Completely random to be unguessable, currently unused

/* TODO: Encryption */
function EncryptUUID(uuid, pin) {
    return uuid;
}

function DecryptUUID(uuid, pin) {
    return uuid;
}

function _requestFirstName(uuid, req) {
    /* TODO: Retrieve First & Last Name from db */
    client.query(`select patient_name from patients where patient_id=${uuid}`, (err,res)=>{
        if(err)
            return;
        req.send(JSON.stringify({ pid: 2, fullname: res.rows[0].patient_name }));
    });
}


const _requestPrescriptionList = function(uuid, req) { // req id 0
    let medicines = [];

    client.query(`select * from patients where patient_id=${uuid}`, (err, res) => {
        if(err) {
            console.log(`No patient with id: ${uuid}`);
            console.log("Error: ", err);
            return;
        }

        if(!res.rows[0]) {
            console.log(res);
            console.log("Invalid row");
            return;
        }

        let all_medicines = res.rows[0].medicine_name;
        let all_dosage = res.rows[0].dosage;
    
        for(let i = 0; i < all_medicines.length; i++)
            medicines.push({ "medicine": all_medicines[i], "dosage": all_dosage[i] });

        req.send(JSON.stringify({ pid: 0, prescriptions: medicines }));
    });

};

const _retrievedPrescription = function(uuid, req) { // req id 1
    /* TODO: Set you've retrieved prescription, and set next refill date */
};

const _addNewAccount = function(uuid, prescriptions, req) {
    /* TODO: Implement */
    const obj = JSON.parse(prescriptions);
    client.query(`insert into patients (patient_id, medicine_name, dosage) values (${uuid}, '${obj.medicine_name}', '${obj.dosage}');`);
    ws.send(JSON.stringify({ pid: -1, encUUID: EncryptUUID(uuid, data.pin) }));
};

wss.on('connection', function(ws, req) {

    ws.onmessage = function(evnt) {

        const data = JSON.parse(evnt.data); // This assumes every packet that gets sent is correct JSON formatting, terrible sanitization, must fix

        if(!data) {
            console.log("['Connection'] Invalid data");
            return;
        }

        if(data.pid == -1) { // Requesting new account, unused
            /*
                { // -1
                    pin = int,
                    firstname: string,
                    lastname: string,
                    hasproofinsurance: bool,
                    prescriptions = [ { medication: string, dosage: int } ],
                    adminkey: int
                }
            */
            if(data.adminkey != SECRET_ADMIN_KEY)
                return;
            /* TODO: Check if patient already exists */
            
            let encryptedPin = (data.pin);

            let uuid = new Number(crypto.randomUUID().replaceAll('-', ''));

            _addNewAccount(uuid, data.prescriptions, ws);
            return;
        }
        
        const pin = data.pin;

        console.log(`Recieved pin: ${pin}`);

        var uuid = data.encUUID;

        console.log(`Recieved encrypted uuid: ${uuid}`);

        uuid = DecryptUUID(uuid);

        console.log(`Decrypted uuid: ${uuid}`);

        switch(data.pid) {
            case 0:
                _requestPrescriptionList(uuid, ws);
                break;
            case 1:
                _retrievedPrescription(uuid, ws);
                break;
            case 2:
                _requestFirstName(uuid, ws);
                break;
        }

    };

});