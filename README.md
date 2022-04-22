# DEPRECATED: This project is no longer maintained

# Welcome to the DegreeC Node

You will need to create a Firebase project and get the auth code, and create a constants.h file in the same directory as DegreeCNode.ino that looks like the following:

    #ifndef Constants_h
    #define Constants_h
        static const char* WIFI_SSID = "";     //YOUR WIFI SSID
        static const char* WIFI_PASS = "";     //YOUR WIFI PASSWORD

        static const String FIREBASE_REF = ""; //A "root" folder
        static const String FIREBASE_HOST = ""; //The host URL
        static const String FIREBASE_AUTH = ""; //The Firebase Auth code
        static const String FIREBASE_VALUE = FIREBASE_REF + "/readings";
        static const String FIREBASE_IP = FIREBASE_REF + "/ip
    #endif
