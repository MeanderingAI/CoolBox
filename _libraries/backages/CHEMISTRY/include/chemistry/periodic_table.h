#pragma once

#include <string>

#pragma once

#include <string>
#include <array>
#include <vector>
#include <algorithm>

namespace chemistry {

struct Element {
    int atomic_number;    // Z
    const char* symbol;   // e.g. "Li"
    const char* name;     // e.g. "Lithium"
    double atomic_weight; // approximate
    int valence_electrons; // outer (valence) electron count when applicable
    int group;            // periodic group (1-18), 0 if unknown
    int period;           // periodic period (1-7), 0 if unknown
    double electronegativity; // Pauling scale, 0.0 if unknown
    const char* note;     // optional short comment for chemistry usage
};

// Full periodic table (1..118). `note` contains brief chemistry-relevant hints
// for elements used in battery/electronics or common chemistry contexts.
static constexpr Element kPeriodicTable[] = {
    {1,  "H",  "Hydrogen",    1.008,    1, 1, 1, 2.20,  "common in acids/protons"},
    {2,  "He", "Helium",      4.0026,   0, 18,1, 0.0,  "inert"},
    {3,  "Li", "Lithium",     6.94,     1, 1, 2, 0.98,  "battery anode (Li-ion)"},
    {4,  "Be", "Beryllium",   9.0122,   2, 2, 2, 1.57,  "toxic, structural alloy"},
    {5,  "B",  "Boron",       10.81,    3, 13,2, 2.04,  "borates, semiconductors"},
    {6,  "C",  "Carbon",      12.011,   4, 14,2, 2.55,  "organic backbone, electrodes"},
    {7,  "N",  "Nitrogen",    14.007,   5, 15,2, 3.04,  "gases, part of electrolytes"},
    {8,  "O",  "Oxygen",      15.999,   6, 16,2, 3.44,  "oxidizer, cathode reactions"},
    {9,  "F",  "Fluorine",    18.998,   7, 17,2, 3.98,  "electrolyte salts (e.g., LiPF6)"},
    {10, "Ne", "Neon",        20.180,   0, 18,2, 0.0,   "inert"},
    {11, "Na", "Sodium",      22.990,   1, 1, 3, 0.93,  "Na-ion batteries, electrolytes"},
    {12, "Mg", "Magnesium",   24.305,   2, 2, 3, 1.31,  "Mg batteries (emerging)"},
    {13, "Al", "Aluminium",   26.982,   3, 13,3, 1.61,  "current collectors, conductors"},
    {14, "Si", "Silicon",     28.085,   4, 14,3, 1.90,  "anode material (Si anodes)"},
    {15, "P",  "Phosphorus",  30.974,   5, 15,3, 2.19,  "phosphate cathodes (LiFePO4)"},
    {16, "S",  "Sulfur",      32.06,    6, 16,3, 2.58,  "Li-S batteries, active material"},
    {17, "Cl", "Chlorine",    35.45,    7, 17,3, 3.16,  "chlorides, reagents"},
    {18, "Ar", "Argon",       39.948,   0, 18,3, 0.0,   "inert"},
    {19, "K",  "Potassium",   39.098,   1, 1, 4, 0.82,  "K-ion batteries (research)"},
    {20, "Ca", "Calcium",     40.078,   2, 2, 4, 1.00,  "materials/compounds"},
    {21, "Sc", "Scandium",    44.956,   2, 3, 4, 1.36,  "alloys"},
    {22, "Ti", "Titanium",    47.867,   4, 4, 4, 1.54,  "structural, corrosion-resistant"},
    {23, "V",  "Vanadium",    50.942,   5, 5, 4, 1.63,  "vanadium redox flow batteries"},
    {24, "Cr", "Chromium",    51.996,   6, 6, 4, 1.66,  "alloys, plating"},
    {25, "Mn", "Manganese",   54.938,   2, 7, 4, 1.55,  "MnO2 cathodes (alkaline), battery chem"},
    {26, "Fe", "Iron",        55.845,   2, 8, 4, 1.83,  "common, part of LiFePO4"},
    {27, "Co", "Cobalt",      58.933,   2, 9, 4, 1.88,  "LiCoO2 cathodes (historical)"},
    {28, "Ni", "Nickel",      58.693,   2, 10,4, 1.91,  "NiMH anodes/cathodes, NMC cathodes"},
    {29, "Cu", "Copper",      63.546,   1, 11,4, 1.90,  "current collectors, conductors"},
    {30, "Zn", "Zinc",        65.38,    2, 12,4, 1.65,  "alkaline anode chemistry (Zn)"},
    {31, "Ga", "Gallium",     69.723,   3, 13,4, 1.81,  "semiconductors"},
    {32, "Ge", "Germanium",   72.630,   4, 14,4, 2.01,  "semiconductors"},
    {33, "As", "Arsenic",     74.922,   5, 15,4, 2.18,  "toxic, semiconductors"},
    {34, "Se", "Selenium",    78.971,   6, 16,4, 2.55,  "photovoltaics, additives"},
    {35, "Br", "Bromine",     79.904,   7, 17,4, 2.96,  "electrolyte components"},
    {36, "Kr", "Krypton",     83.798,   0, 18,4, 0.0,   "inert"},
    {37, "Rb", "Rubidium",    85.468,   1, 1, 5, 0.82,  "research batteries"},
    {38, "Sr", "Strontium",   87.62,    2, 2, 5, 0.95,  "phosphors"},
    {39, "Y",  "Yttrium",     88.906,   3, 3, 5, 1.22,  "alloys, catalysts"},
    {40, "Zr", "Zirconium",   91.224,   4, 4, 5, 1.33,  "corrosion-resistant"},
    {41, "Nb", "Niobium",     92.906,   5, 5, 5, 1.6,   "superconductors, alloys"},
    {42, "Mo", "Molybdenum",  95.95,    6, 6, 5, 2.16,  "alloys, catalysts"},
    {43, "Tc", "Technetium",  98.0,     0, 7, 5, 0.0,   "radioactive"},
    {44, "Ru", "Ruthenium",   101.07,   0, 8, 5, 2.2,   "catalysts"},
    {45, "Rh", "Rhodium",     102.91,   0, 9, 5, 2.28,  "catalysts"},
    {46, "Pd", "Palladium",   106.42,   0, 10,5, 2.20,  "catalysts, hydrogen storage"},
    {47, "Ag", "Silver",      107.87,   1, 11,5, 1.93,  "conductive, contacts"},
    {48, "Cd", "Cadmium",     112.41,   2, 12,5, 1.69,  "NiCd batteries (legacy, toxic)"},
    {49, "In", "Indium",      114.82,   3, 13,5, 1.78,  "touchscreens"},
    {50, "Sn", "Tin",         118.71,   4, 14,5, 1.96,  "solders, electrodes"},
    {51, "Sb", "Antimony",    121.76,   5, 15,5, 2.05,  "alloys, flame retardants"},
    {52, "Te", "Tellurium",   127.60,   6, 16,5, 2.1,   "thermoelectrics"},
    {53, "I",  "Iodine",      126.90,   7, 17,5, 2.66,  "disinfectants, reagents"},
    {54, "Xe", "Xenon",       131.29,   0, 18,5, 0.0,   "inert"},
    {55, "Cs", "Caesium",     132.91,   1, 1, 6, 0.79,  "ion propulsion, research"},
    {56, "Ba", "Barium",      137.33,   2, 2, 6, 0.89,  "phosphors"},
    {57, "La", "Lanthanum",   138.91,   3, 3, 6, 1.10,  "lanthanides"},
    {58, "Ce", "Cerium",      140.12,   4, 4, 6, 1.12,  "lanthanides"},
    {59, "Pr", "Praseodymium",140.91,   0, 0, 0, 0.0,   "lanthanides"},
    {60, "Nd", "Neodymium",   144.24,   0, 0, 0, 1.14,  "magnets"},
    {61, "Pm", "Promethium",  145.0,    0, 0, 0, 0.0,   "radioactive"},
    {62, "Sm", "Samarium",    150.36,   0, 0, 0, 1.17,  "magnets"},
    {63, "Eu", "Europium",    151.96,   0, 0, 0, 1.2,   "phosphors"},
    {64, "Gd", "Gadolinium",  157.25,   0, 0, 0, 1.2,   "MRI contrast"},
    {65, "Tb", "Terbium",    158.93,   0, 0, 0, 1.2,   "phosphors"},
    {66, "Dy", "Dysprosium",  162.50,   0, 0, 0, 1.22,  "magnets"},
    {67, "Ho", "Holmium",     164.93,   0, 0, 0, 1.23,  "specialty alloys"},
    {68, "Er", "Erbium",      167.26,   0, 0, 0, 1.24,  "lasers"},
    {69, "Tm", "Thulium",     168.93,   0, 0, 0, 1.25,  "lasers"},
    {70, "Yb", "Ytterbium",   173.05,   0, 0, 0, 1.1,   "specialty alloys"},
    {71, "Lu", "Lutetium",    174.97,   0, 0, 0, 1.27,  "lanthanides"},
    {72, "Hf", "Hafnium",     178.49,   4, 4, 6, 1.3,   "alloys, control rods"},
    {73, "Ta", "Tantalum",    180.95,   5, 5, 6, 1.5,   "capacitors, corrosion-resistant"},
    {74, "W",  "Tungsten",    183.84,   6, 6, 6, 2.36,  "high-strength, contacts"},
    {75, "Re", "Rhenium",     186.21,   7, 7, 6, 1.9,   "superalloys"},
    {76, "Os", "Osmium",      190.23,   8, 8, 6, 2.2,   "dense, limited use"},
    {77, "Ir", "Iridium",     192.22,   9, 9, 6, 2.2,   "contacts, catalysts"},
    {78, "Pt", "Platinum",    195.08,   10,10,6,2.28,  "catalysts"},
    {79, "Au", "Gold",        196.97,   11,11,6,2.54,  "conductive, contacts"},
    {80, "Hg", "Mercury",     200.59,   0, 12,6,2.0,   "toxic, legacy uses"},
    {81, "Tl", "Thallium",    204.38,   3, 13,6,1.62,  "toxic"},
    {82, "Pb", "Lead",        207.2,    4, 14,6,2.33,  "lead-acid batteries (Pb)"},
    {83, "Bi", "Bismuth",     208.98,   5, 15,6,2.02,  "alloys"},
    {84, "Po", "Polonium",    209.0,    6, 16,6,2.0,   "radioactive"},
    {85, "At", "Astatine",    210.0,    7, 17,6,2.2,   "radioactive"},
    {86, "Rn", "Radon",       222.0,    0, 18,6,0.0,   "radioactive gas"},
    {87, "Fr", "Francium",    223.0,    1, 1, 7,0.7,   "radioactive"},
    {88, "Ra", "Radium",      226.0,    2, 2, 7,0.9,   "radioactive"},
    {89, "Ac", "Actinium",    227.0,    0, 0, 0,0.0,   "radioactive"},
    {90, "Th", "Thorium",     232.04,   0, 0, 0,0.0,   "radioactive"},
    {91, "Pa", "Protactinium",231.04,   0, 0, 0,0.0,   "radioactive"},
    {92, "U",  "Uranium",     238.03,   0, 0, 0,1.38,  "nuclear"},
    {93, "Np", "Neptunium",   237.0,    0, 0, 0,0.0,   "radioactive"},
    {94, "Pu", "Plutonium",   244.0,    0, 0, 0,1.28,  "radioactive"},
    {95, "Am", "Americium",   243.0,    0, 0, 0,0.0,   "radioactive"},
    {96, "Cm", "Curium",      247.0,    0, 0, 0,0.0,   "radioactive"},
    {97, "Bk", "Berkelium",   247.0,    0, 0, 0,0.0,   "radioactive"},
    {98, "Cf", "Californium", 251.0,    0, 0, 0,0.0,   "radioactive"},
    {99, "Es", "Einsteinium", 252.0,    0, 0, 0,0.0,   "radioactive"},
    {100,"Fm", "Fermium",     257.0,    0, 0, 0,0.0,   "radioactive"},
    {101,"Md", "Mendelevium", 258.0,    0, 0, 0,0.0,   "radioactive"},
    {102,"No", "Nobelium",    259.0,    0, 0, 0,0.0,   "radioactive"},
    {103,"Lr", "Lawrencium",  266.0,    0, 0, 0,0.0,   "radioactive"},
    {104,"Rf", "Rutherfordium",267.0,   0, 0, 0,0.0,   "synthetic"},
    {105,"Db", "Dubnium",     268.0,    0, 0, 0,0.0,   "synthetic"},
    {106,"Sg", "Seaborgium",  269.0,    0, 0, 0,0.0,   "synthetic"},
    {107,"Bh", "Bohrium",     270.0,    0, 0, 0,0.0,   "synthetic"},
    {108,"Hs", "Hassium",     270.0,    0, 0, 0,0.0,   "synthetic"},
    {109,"Mt", "Meitnerium",  278.0,    0, 0, 0,0.0,   "synthetic"},
    {110,"Ds", "Darmstadtium",281.0,    0, 0, 0,0.0,   "synthetic"},
    {111,"Rg", "Roentgenium",  282.0,    0, 0, 0,0.0,   "synthetic"},
    {112,"Cn", "Copernicium",  285.0,    0, 0, 0,0.0,   "synthetic"},
    {113,"Nh", "Nihonium",    286.0,    0, 0, 0,0.0,   "synthetic"},
    {114,"Fl", "Flerovium",   289.0,    0, 0, 0,0.0,   "synthetic"},
    {115,"Mc", "Moscovium",   290.0,    0, 0, 0,0.0,   "synthetic"},
    {116,"Lv", "Livermorium", 293.0,    0, 0, 0,0.0,   "synthetic"},
    {117,"Ts", "Tennessine",  294.0,    0, 0, 0,0.0,   "synthetic"},
    {118,"Og", "Oganesson",   294.0,    0, 0, 0,0.0,   "synthetic"}
};

inline Element element_by_symbol(const std::string& sym) {
    for (const auto &e : kPeriodicTable) {
        if (sym == e.symbol) return e;
    }
    return {0, "?", "Unknown", 0.0, 0, 0, 0, 0.0, ""};
}

inline Element element_by_atomic_number(int z) {
    for (const auto &e : kPeriodicTable) {
        if (e.atomic_number == z) return e;
    }
    return {0, "?", "Unknown", 0.0, 0, 0, 0, 0.0, ""};
}

inline std::vector<Element> elements_from_symbols(const std::vector<std::string>& syms) {
    std::vector<Element> out;
    out.reserve(syms.size());
    for (const auto &s : syms) out.push_back(element_by_symbol(s));
    return out;
}

} // namespace chemistry
