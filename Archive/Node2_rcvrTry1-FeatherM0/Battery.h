// FUTURE DEVELOPMENT Battery info
const long InternalReferenceVoltage = 1062;  // Adjust this value to your board's specific internal BG voltage
//TODO verify this number...
int A0adc;
float battVolts;

#define VBATPIN A7

double readValues() {
    // Payload format Bytes: [(Bat-Voltage - 2) * 100]
    double measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    Serial.print("VBat: " ); Serial.println(measuredvbat);
    measuredvbat -= 2; // offset by -2
    measuredvbat *= 100; //make it centi volts
    Serial.print("VBat in CVolts-2: " ); Serial.println(measuredvbat);
    return measuredvbat;
}
