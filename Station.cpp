/*
 * Station.cpp
 *
 *  Created on: Jan 30, 2013
 *      Author: x-warrior
 */

#include "Station.h"

Station::Station() {
    GenericStation();
    flag = NOTHING;
    forward = 0;
    for (int i = 0; i < 5; i++) {
        idSelection[i] = 0;
        levelSelection[i] = 0;
    }
    control = 0;

    Serial.println("DISCOVER WHO IS LISTENING");
    sendWhoListen();
    long sent_time = millis();
    while ((millis() - sent_time) < (256 * 100)) {
        readProtocol();
    }
    //Serial.println("Finish discovering who is listening");
    //Serial.println("ASKING FOR CONFIGURATION");
    sendAskConfig();
    sent_time = millis();
    while ((millis() - sent_time) < (256 * 100)) {
        readProtocol();
    }
    Serial.println("Finish ASKING FOR CONFIGURATION");
    Serial.println("SETUP START REGULAR TASK");
    print();
}

void Station::sendWhoListen() {
    PMessage c = PMessage(PMessage::PROTOCOL, PMessage::WHO_LISTEN, BROADCAST,
            (uint8_t) 0x00, (uint8_t) 0x00, (uint8_t) 0x00, (uint8_t) 0x00);
    writeProtocol(c);
}

void Station::sendAskConfig() {
    uint8_t temp = 0;
    uint8_t tempId = 0;
    flag = WAITING;
    for (int x = 0; x < control; x++) {
        for (int y = 0; y < control - 1; y++) {
            if (levelSelection[y] > levelSelection[y + 1]) {
                temp = levelSelection[y + 1];
                tempId = idSelection[y + 1];
                levelSelection[y + 1] = levelSelection[y];
                idSelection[y + 1] = idSelection[y];
                levelSelection[y] = temp;
                idSelection[y] = tempId;
            }
        }
    }
    Serial.print(idSelection[0], HEX);
    Serial.print(" ");
    Serial.print(idSelection[1], HEX);
    Serial.print(" ");
    Serial.print(idSelection[2], HEX);
    Serial.print(" ");
    Serial.print(idSelection[3], HEX);
    Serial.print(" ");
    Serial.print(idSelection[4], HEX);
    Serial.print(" ");
    PMessage c = 0;
    // THIS IF IS JUST FOR TESTING PURPOSE!
    /*if (idSelection[1] != 0) {
     c = PMessage(PMessage::PROTOCOL, PMessage::ASK_CONFIG, idSelection[1],
     idSelection[0], idSelection[2], idSelection[3], idSelection[4]);
     } else {*/
    c = PMessage(PMessage::PROTOCOL, PMessage::ASK_CONFIG, idSelection[0],
                idSelection[1], idSelection[2], idSelection[3], idSelection[4]);
    if(idSelection[0] == 0x01) {
        c.proto = SET_MSG_DEST(c.proto, 1);
    }

    //}
    writeProtocol(c);
}

void Station::receivedWhoListen(PMessage p) {
    if (id != 0) {
        long wait = millis();
        while ((millis() - wait) < (id * 100)) {
            delay(5);
        }
        PMessage c = PMessage(PMessage::PROTOCOL, PMessage::I_LISTEN,
                (uint8_t) 0x00, (uint8_t) id, 0x00, (uint8_t) level,
                (uint8_t) 0x00);
        writeProtocol(c);
    }
}

void Station::receivedIListen(PMessage p) {
    if (p.id_dest == 0x00 && id == 0x00) {
        if (control > 4) {
            int bigger;
            uint8_t temp = 0;
            for (int i = 0; i < 4; i++) {
                if (levelSelection[i] > temp) {
                    temp = levelSelection[i];
                    bigger = i;
                }
            }
            if (p.value2 < levelSelection[bigger]) {
                levelSelection[bigger] = p.value2;
                idSelection[bigger] = p.id_from;
            }
        } else {
            idSelection[control] = p.id_from;
            levelSelection[control] = p.value2;
            control++;
            Serial.print("CONTROL: ");
            Serial.println(control);
        }
    }
}

void Station::receivedAskConfig(PMessage p) {
    if (p.id_dest == id || indirectChild(p.id_dest)) {
        flag = RETRANSMIT;
        if (parentPipe == 0x01) {
            p.proto = SET_MSG_DEST(p.proto, 1);
        }
        if(p.id_dest == id) {
            flag = RETRANSMIT_FIRST;
        }
        Serial.println("retransmit ASK_CONFIG");
        //forward = p.id_dest;
        //p.id_dest = parentPipe;
        writeProtocol(p);
    }

}

void Station::receivedSetConfig(PMessage p) {
    if (flag == RETRANSMIT || flag == RETRANSMIT_FIRST) {
        Serial.println("retransmit SET_CONFIG");
        flag = NOTHING;
        //p.id_dest = forward;
        if (p.value3 == id) {
            // I'm his parent, open new pipe;
            registerPipe(findOpenPipe(), p.value);
        }
        if (indirectChild(p.value3)) {
            registerIndirecChild(p.value3, p.value);
        }
        if(flag == RETRANSMIT_FIRST) {
            p.proto = SET_MSG_DEST(p.proto, 1);
        }
        writeProtocol(p);
        print();
    } else if (flag == WAITING && GET_MSG_DEST(p.proto) == 1) {
        Serial.println("Received set_config");
        flag = NOTHING;
        parentPipe = p.value3; //c.id_from;
        id = p.value;
        level = p.value2;
        radio.openReadingPipe(1, ID_TO_PIPE(id));
    } else if (flag == NOTHING) {
        if (p.value3 == id) {
            Serial.println("REGISTER CHILD");
            // I'm his parent, open new pipe;
            registerPipe(findOpenPipe(), p.value);
        }
        if (indirectChild(p.value3)) {
            registerIndirecChild(p.value3, p.value);
        }
        print();
    }
}

Station::~Station() {
// TODO Auto-generated destructor stub
}
