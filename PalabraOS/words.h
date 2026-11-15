// =============================================================
//  PalabraOS - word list
//
//  One line per word:   { "spanish", "english", "example sentence" },
//
//  - Accents work: just type them (á é í ó ú ü ñ ¿ ¡).
//  - The example sentence is optional - use "" to leave it out.
//  - New words are taught in the order they appear here, so put
//    the most useful words first.
//  - To ADD words later, put them at the END (your progress is kept).
//  - If you REPLACE or reorder the list, add 1 to WORD_LIST_VERSION
//    so the old progress is cleared instead of sticking to the wrong words.
// =============================================================
#pragma once

#define WORD_LIST_VERSION 1

struct Word {
  const char* es;   // Spanish
  const char* en;   // English
  const char* ex;   // example sentence in Spanish (may be "")
};

const Word WORDS[] = {
  // --- connectors & adverbs ---
  { "aunque",          "although",            "Salgo aunque llueve." },
  { "todavía",         "still / yet",         "Todavía no he comido." },
  { "sin embargo",     "however",             "Es caro; sin embargo, me gusta." },
  { "a menudo",        "often",               "Voy al gimnasio a menudo." },
  { "de repente",      "suddenly",            "De repente empezó a llover." },
  { "además",          "besides / also",      "Además, es muy barato." },
  { "apenas",          "hardly",              "Apenas tengo tiempo libre." },
  { "quizás",          "maybe",               "Quizás vaya mañana." },
  { "por lo menos",    "at least",            "Duerme por lo menos siete horas." },
  { "en cuanto",       "as soon as",          "Te llamo en cuanto llegue." },

  // --- verbs ---
  { "conseguir",       "to get / to achieve", "Quiero conseguir un buen trabajo." },
  { "quedarse",        "to stay",             "Me quedo en casa esta noche." },
  { "darse cuenta",    "to realize",          "Me di cuenta del error." },
  { "echar de menos",  "to miss (someone)",   "Echo de menos a mi familia." },
  { "aprovechar",      "to make the most of", "Hay que aprovechar el tiempo." },
  { "elegir",          "to choose",           "Tienes que elegir un color." },
  { "intentar",        "to try",              "Voy a intentarlo otra vez." },
  { "olvidar",         "to forget",           "No olvides las llaves." },
  { "recordar",        "to remember",         "No recuerdo su nombre." },
  { "mejorar",         "to improve",          "Quiero mejorar mi español." },
  { "alquilar",        "to rent",             "Quiero alquilar un piso." },
  { "ahorrar",         "to save (money)",     "Ahorro dinero cada mes." },
  { "gastar",          "to spend (money)",    "Gasto mucho en comida." },
  { "enviar",          "to send",             "Te envío un mensaje luego." },
  { "recoger",         "to pick up",          "Voy a recoger a mi hermano." },
  { "apagar",          "to turn off",         "Apaga la luz, por favor." },
  { "encender",        "to turn on",          "Enciende el ordenador." },
  { "arreglar",        "to fix",              "Tengo que arreglar la bici." },
  { "probar",          "to try / to taste",   "¿Quieres probar la sopa?" },
  { "tardar",          "to take (time)",      "El tren tarda dos horas." },

  // --- nouns ---
  { "la pantalla",     "screen",              "La pantalla está rota." },
  { "el botón",        "button",              "Pulsa el botón rojo." },
  { "el enchufe",      "plug / socket",       "¿Dónde hay un enchufe?" },
  { "la herramienta",  "tool",                "Necesito una herramienta." },
  { "la factura",      "bill / invoice",      "Tengo que pagar la factura." },
  { "el sueldo",       "salary",              "Mi sueldo no es muy alto." },
  { "la cita",         "appointment / date",  "Tengo una cita con el médico." },
  { "el ayuntamiento", "town hall",           "El ayuntamiento está en la plaza." },
  { "la esquina",      "corner",              "La farmacia está en la esquina." },
  { "el billete",      "ticket / banknote",   "Compré un billete de tren." },
  { "el equipaje",     "luggage",             "Mi equipaje pesa mucho." },
  { "la mascota",      "pet",                 "Mi mascota es un gato." },
  { "el vecino",       "neighbor",            "Mi vecino es muy amable." },
  { "la costumbre",    "habit / custom",      "Es una costumbre española." },
  { "el consejo",      "advice",              "Gracias por el consejo." },
  { "la ventaja",      "advantage",           "Hablar español es una ventaja." },
  { "el ruido",        "noise",               "Hay mucho ruido en la calle." },
  { "la tormenta",     "storm",               "Esta noche hay tormenta." },

  // --- adjectives ---
  { "cansado",         "tired",               "Estoy muy cansado hoy." },
  { "ocupado",         "busy",                "Hoy estoy muy ocupado." },
  { "barato",          "cheap",               "Este móvil es muy barato." },
  { "amable",          "kind",                "La profesora es muy amable." },
  { "sencillo",        "simple",              "El examen fue sencillo." },
  { "peligroso",       "dangerous",           "Es peligroso nadar aquí." },
  { "orgulloso",       "proud",               "Estoy orgulloso de ti." },
  { "extraño",         "strange",             "Oí un ruido extraño." },
  { "lleno",           "full",                "El vaso está lleno." },
  { "vacío",           "empty",               "El restaurante está vacío." },
};

const uint16_t WORD_COUNT = sizeof(WORDS) / sizeof(WORDS[0]);
