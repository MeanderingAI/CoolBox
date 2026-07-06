# Solid-State Battery Manufacturing Procedures

## Overview
This document describes the step-by-step procedures for manufacturing solid-state batteries. Unlike conventional lithium-ion batteries, solid-state batteries use solid electrolytes instead of liquid electrolyte solutions, enabling higher energy density and improved safety.

## Battery Types Covered

- **Polymer Electrolyte Batteries**: Solid polymer electrolyte (SPE)
- **Ceramic Electrolyte Batteries**: Oxide or sulfide ceramics (LLZO, Li₇La₃Zr₂O₁₂, LGPS)
- **Composite Electrolytes**: Hybrid ceramic-polymer systems

## Required Materials Checklist

### Anode Materials
- [ ] Lithium metal foil (0.5-2 mm thickness, 99.9% purity)
- [ ] Or graphite anode (for hybrid systems)
- [ ] Anode current collector (copper or stainless steel substrate)
- [ ] Protective coatings (optional: thin polymer layer)

### Cathode Materials
- [ ] Cathode powder (LiCoO₂, NMC, NCA, LiFePO₄)
- [ ] Binder for cathode composite
- [ ] Conductive additive (carbon black or graphene)
- [ ] Aluminum foil current collector

### Solid Electrolyte Materials

**For Polymer Electrolytes:**
- [ ] Polymer base (PEO, PVDF, PMMA)
- [ ] Lithium salt (LiClO₄, LiTFSI, LiBF₄)
- [ ] Plasticizers (ethylene carbonate, propylene carbonate)
- [ ] Solvent (dimethylformamide - DMF, DMSO)

**For Ceramic Electrolytes:**
- [ ] LLZO powder (Li₇La₃Zr₂O₁₂) or equivalent oxide
- [ ] Raw materials (LiOH, La₂O₃, ZrO₂)
- [ ] Sintering aids (optional)
- [ ] Pre-sintered pellets (if outsourced)

**For Composite Electrolytes:**
- [ ] Ceramic particles (LLZO, Li₃N)
- [ ] Polymer binder (PVDF)
- [ ] Lithium salt
- [ ] Solvent

### Packaging & Assembly
- [ ] Cell casing (stainless steel or aluminum)
- [ ] Sealing gaskets
- [ ] Terminals (current collectors)
- [ ] Pressure plates (for contact improvement)
- [ ] Thermal interface material (TIM)

---

## Phase 1: Solid Electrolyte Fabrication

### Step 1.1: Polymer Electrolyte Preparation

**For PEO-based Polymer Electrolytes:**

**Materials:**
- Polyethylene oxide (PEO) Mw = 100,000-600,000 g/mol
- Lithium salt (LiClO₄): 4-16 wt% of PEO
- Plasticizer (EC): 5-20 wt% of total
- Solvent (DMF or DMSO)

**Procedure:**

1. **Material Preparation**
   - Weigh PEO pellets/powder: target mass M
   - Weigh LiClO₄: 0.12 × M (12% typical)
   - Weigh EC: 0.10 × M (10% typical)
   - Dry all components at 80°C under vacuum for 4 hours
   - Cool to room temperature

2. **Electrolyte Casting**
   - Dissolve PEO in warm DMF (60°C) with gentle stirring
   - Concentration: 5-10% PEO by weight
   - Mix for 2-4 hours at 400-600 RPM
   - Allow any undissolved material to settle (30 min)
   - Dissolve LiClO₄ in minimal DMF
   - Add slowly to PEO solution with stirring
   - Add plasticizer (EC)
   - Stir for 30 minutes at room temperature
   - Allow to equilibrate for 2-4 hours

3. **Gel Formation**
   - Pour solution onto silicone substrate
   - Flatten to uniform ~1 mm thickness
   - Allow solvent evaporation at room temperature for 24 hours
   - Increase temperature to 40°C, hold 12 hours
   - Increase to 60°C, hold 12 hours
   - Final drying at 80°C for 24 hours under vacuum
   - Resulting thickness: 200-500 μm

4. **Electrolyte Properties Verification**
   - Ionic conductivity @ 20°C: 10⁻⁵ to 10⁻⁶ S/cm (before conditioning)
   - After conditioning at 60°C: 10⁻⁴ to 10⁻³ S/cm
   - Mechanical strength: tensile strength > 1 MPa
   - Thermal stability: stable to 150°C

### Step 1.2: Ceramic Electrolyte Preparation

**For LLZO (Garnet-type) Electrolytes:**

**Materials:**
- Lithium hydroxide (LiOH·H₂O): high purity
- Lanthanum oxide (La₂O₃): 99.9% purity
- Zirconium oxide (ZrO₂): 99.9% purity
- Dopant (if applicable): Y₂O₃, Ta₂O₅

**Procedure:**

1. **Solid-State Synthesis (Conventional Solid-State Route)**

   **Step 1: Precursor Preparation**
   - Weigh LiOH·H₂O: 4.00 g (calculate from Li₇La₃Zr₂O₁₂ stoichiometry)
   - Weigh La₂O₃: 4.72 g
   - Weigh ZrO₂: 1.51 g
   - Pre-dry La₂O₃ and ZrO₂ at 900°C for 4 hours
   - Cool in desiccator

   **Step 2: Mixing**
   - Mix metal oxides in ball mill
   - Ball mill speed: 300 RPM
   - Time: 4-6 hours
   - Ball material: yttria-stabilized zirconia (YSZ)
   - Ethanol as milling medium

   **Step 3: First Heat Treatment**
   - Calcination at 500°C for 2 hours (ramp: 2°C/min)
   - Cool to room temperature
   - Grind thoroughly with mortar and pestle
   - Sieve through 200 mesh

   **Step 4: Second Heat Treatment**
   - Heat treat at 750°C for 4 hours (ramp: 2°C/min)
   - Cool naturally to room temperature
   - Grind again

   **Step 5: Final Sintering**
   - Press into pellet: 5-10 mm diameter
   - Apply 50-100 MPa pressure
   - Heat treat at 900-1000°C for 8-16 hours
   - Ramp rate: 2°C/min, cool naturally
   - Resulting density: 85-95% of theoretical

2. **Ceramic Properties Verification**
   - X-ray diffraction: confirm LLZO cubic phase
   - Density measurement: should be 5.0-5.1 g/cm³
   - Ionic conductivity @ 25°C: 10⁻⁶ S/cm
   - Ionic conductivity @ 60°C: 10⁻⁴ S/cm
   - Tensile strength: > 50 MPa

3. **Pellet Finishing**
   - Grind pellet edges to remove burrs
   - Polish surface with 600-1200 grit sandpaper
   - Clean with deionized water
   - Dry at 120°C for 2 hours
   - Store in desiccator until use

### Step 1.3: Composite Electrolyte Preparation

**For Ceramic-Polymer Composites:**

**Materials:**
- LLZO ceramic particles: 20-60 vol%
- PVDF binder: 20-40 wt% of polymer content
- Lithium salt (LiTFSI): 5-10 wt%
- Solvent (DMF or DMSO)

**Procedure:**

1. **Ceramic Particle Preparation**
   - Crush LLZO sintered pellet to powder
   - Grind in ball mill at 300 RPM for 4 hours
   - Dry at 200°C under vacuum for 4 hours
   - Sieve to obtain 0.1-2 μm particles

2. **Composite Mixing**
   - Weigh ceramic powder: M g
   - Weigh PVDF: 0.30 × M (30% typical)
   - Weigh LiTFSI: 0.07 × M (7% typical)
   - Dissolve PVDF in hot DMF (80°C): 5-10% PVDF
   - Allow to cool to 60°C
   - Add LiTFSI, stir 15 minutes
   - Add ceramic particles gradually with vigorous stirring
   - Mix at 600 RPM for 60 minutes
   - Degas under vacuum for 20 minutes

3. **Composite Casting**
   - Pour onto silicone mold
   - Allow slow evaporation at 40°C for 24 hours
   - Increase to 80°C for 12 hours
   - Final vacuum dry at 120°C for 24 hours
   - Thickness: 300-800 μm
   - Properties: combines ceramic hardness with polymer flexibility

---

## Phase 2: Cathode Preparation

### Step 2.1: Cathode Composite Layer Formation

**For solid-state batteries, cathode is typically a composite layer with solid electrolyte:**

**Materials:**
- Cathode powder (NMC, LiFePO₄): 60-80 wt%
- Solid electrolyte powder (LLZO or polymer): 15-30 wt%
- Binder (PVDF or polyvinylidene fluoride): 5-10 wt%
- Carbon additive: 3-5 wt%
- Solvent (NMP or DMF)

**Procedure:**

1. **Material Preparation**
   - Weigh cathode powder: M grams (target: 70% composite)
   - Weigh solid electrolyte: 0.25 × M
   - Weigh binder: 0.08 × M
   - Weigh carbon additive: 0.04 × M
   - Dry all at 150°C under vacuum for 4 hours

2. **Composite Slurry Preparation**
   - Dissolve PVDF in NMP (80°C, stirring 30 min)
   - Cool to 60°C
   - Add carbon additive
   - Mix at 500 RPM for 10 minutes
   - Add solid electrolyte powder
   - Mix at 600 RPM for 15 minutes
   - Add cathode powder gradually
   - Increase to 800 RPM
   - Mix for 45-60 minutes
   - Consistency check: uniform, pourable, no settling

3. **Cathode Coating onto Current Collector**
   - Aluminum foil substrate: 15-20 μm thick
   - Clean with ethanol
   - Mount on doctor blade coater
   - Apply composite slurry
   - Target wet thickness: 200-400 μm
   - Coating speed: 1-2 m/min
   - Dry immediately at 80°C for 30 minutes
   - Secondary dry at 120°C for 4 hours
   - Final thickness (dry): 50-100 μm

4. **Cathode Densification**
   - Calendering: optional for improved contact
   - Pressure: 50-100 bar
   - Temperature: room temperature or 60°C
   - Speed: 1 m/min
   - Results in 10-20% thickness reduction

---

## Phase 3: Anode Preparation

### Step 3.1: Lithium Metal Anode

**For solid-state batteries with lithium metal anode:**

**Materials:**
- Lithium metal foil: 99.9% purity
- Thickness: 0.5-2 mm (typically 0.75 mm)
- Protective coating material (optional)

**Procedure:**

1. **Lithium Metal Handling**
   - Remove from storage oil/mineral oil container
   - Wipe excess oil with dry, lint-free cloth
   - Handle only with stainless steel tweezers
   - Avoid moisture exposure: < 1 second in ambient air
   - Perform operations in dry glove box (< 1 ppm H₂O)

2. **Surface Treatment** (optional but recommended)
   - Create thin protective coating to prevent oxidation during assembly
   - Option A: Deposit 10-100 nm solid electrolyte layer
     - Sputter coating of LLZO (30 seconds to 1 minute)
     - In-situ polymerization of ionic liquid polymer
   - Option B: Chemical passivation
     - Brief exposure to inert atmosphere with trace O₂ (~10 ppm)
     - Forms thin Li₂O₂ and Li₂CO₃ layer
   - Inspect surface: should appear silvery, not dull

3. **Anode Preparation for Assembly**
   - Cut lithium foil to required dimensions
   - Laminate onto current collector (stainless steel, copper, or nickel)
   - Use adhesive (optional thin PVDF layer): < 10 μm
   - Total thickness: 0.6-2.1 mm
   - Store under argon until assembly

### Step 3.2: Graphite Anode (Alternative)

**For hybrid solid-state batteries using graphite instead of lithium metal:**

**Materials:**
- Graphite powder: natural or synthetic, 10-20 μm
- Binder: PVDF or CMC
- Conductive additive: carbon black
- Solvent (NMP for PVDF, water for CMC)

**Procedure:**

- Prepare similar to lithium-ion anode preparation
- See Lithium-Ion Battery Manufacturing section, Step 1.2
- Key difference: solid electrolyte coating replaces liquid electrolyte compatibility

---

## Phase 4: Cell Assembly

### Step 4.1: Solid-State Cell Stacking

**Layer Assembly Order (for pouch cell example):**

1. **Negative Terminal** (stainless steel or nickel current collector)
2. **Anode** (lithium metal foil or graphite composite)
3. **Anode-Electrolyte Interface** (optional: thin protective SEI-forming layer)
4. **Solid Electrolyte** (polymer, ceramic, or composite)
5. **Cathode-Electrolyte Interface** (optional: cathode protective coating)
6. **Cathode** (oxide composite layer)
7. **Positive Terminal** (aluminum or composite current collector)
8. **Positive Tab**

**Procedure:**

1. **Preparation in Glove Box**
   - All components must be in dry glove box (< 1 ppm H₂O, < 1 ppm O₂)
   - Pre-stage all materials
   - Temperature: 20-25°C
   - Use clean stainless steel tweezers and spatulas

2. **Layer-by-Layer Assembly**

   **Step 1: Substrate Preparation**
   - Place negative current collector on clean, flat surface
   - Dimensions: slightly larger than electrolyte area
   - Clean surface with dry cloth

   **Step 2: Anode Installation**
   - Position lithium foil on current collector
   - Use light pressure to ensure contact
   - Verify full coverage of surface
   - Remove any wrinkles carefully (do not tear)

   **Step 3: Interface Coating** (if used)
   - Deposit thin protective layer on anode
   - Thickness: 10-50 nm
   - Cover entire anode surface
   - Smooth any irregularities

   **Step 4: Solid Electrolyte Placement**
   - Position solid electrolyte sheet/pellet
   - For pellet: ensure flat surfaces
   - For composite: ensure thickness uniformity
   - Dimensions: minimum overlap on all edges (5 mm typical)

   **Step 5: Cathode Placement**
   - Position cathode composite layer
   - Align with positive current collector area
   - Ensure full contact with solid electrolyte
   - Check for wrinkles or separation

   **Step 6: Positive Terminal Installation**
   - Place positive current collector
   - Apply light pressure for contact
   - Verify alignment with cathode

3. **Stack Verification**
   - Visual inspection: all layers aligned and flat
   - Thickness measurement at multiple points
   - No visible gaps or separations
   - Total thickness: 1-3 mm typical

### Step 4.2: Pressure Application

**Critical for solid-state batteries to ensure ionic contact:**

1. **Mechanical Pressure System**
   - Apply uniaxial pressure to entire stack
   - Pressure: 10-100 MPa (cell design dependent)
   - Typical starting pressure: 20-50 MPa
   - Maintain during formation and cycling

2. **Pressure Application Methods**
   - **Clamping System**: Stainless steel or aluminum frames
   - **Spring-loaded Holders**: For prototypes
   - **Pneumatic Presses**: For higher-volume production
   - **Hydrostatic Pressure**: For uniform compression

3. **Pressure Monitoring**
   - Use pressure sensors at 2-4 points
   - Record pressure values
   - Maintain uniformity: < 10% variation across cell
   - Monitor pressure drift during cycling

### Step 4.3: Sealing

**For Pouch-Type Solid-State Cells:**

1. **Pouch Preparation**
   - Aluminum-plastic laminate pouch
   - Interior: aluminum foil, 10-15 μm
   - Exterior: plastic film
   - Dimensions: 10% larger than assembled stack

2. **Stack Insertion**
   - Place pressurized stack into pouch
   - Ensure centered position
   - Connect terminals to pouch tabs

3. **Pouch Sealing**
   - Heat seal 3 edges at 140-160°C
   - Use impulse sealer or thermal press
   - Sealing width: 5-10 mm
   - Leave top edge unsealed for current lead connection

4. **Current Lead Attachment**
   - Spot weld positive and negative terminals to pouch tabs
   - Weld temperature: 1200-1400°C
   - Pressure: 50-100 MPa
   - Weld time: 0.1-0.2 seconds

5. **Final Sealing**
   - Heat seal top edge
   - Ensure complete seal around terminals
   - Perform leak test: submerge in mineral oil, check for bubbles

---

## Phase 5: Formation & Testing

### Step 5.1: Initial Electrochemical Formation

**Formation procedure differs significantly from liquid-state batteries:**

1. **Low-Temperature Conditioning**
   - Temperature: 50-80°C (not room temperature)
   - Heating time: 4-8 hours at target temperature
   - Purpose: improves ionic conductivity of solid electrolyte
   - Maintain under applied pressure

2. **Initial Charge Formation**
   - Charge voltage: 0.5 V (very low)
   - Current: 0.01C (very low)
   - Time: 4-8 hours
   - Monitor current and voltage continuously
   - Temperature should remain stable

3. **Voltage Ramp Formation**
   - Slow, gradual voltage increase
   - Ramp rate: 5-10 mV per hour
   - Target final voltage: 3.5-4.0 V (depends on cell chemistry)
   - Total ramp time: 24-48 hours
   - Monitor for any unusual voltage drops (indicating shorts)

4. **Electrochemical Impedance Spectroscopy (EIS)**
   - Measure impedance at each voltage step
   - Frequency range: 1 MHz to 1 Hz
   - Track: interface resistance, bulk resistance
   - Record baseline impedance for later comparison

5. **First Discharge**
   - Discharge at 0.05C to lower voltage cutoff
   - Lower cutoff: 2.0-2.5 V (depends on chemistry)
   - Measure capacity: expect 30-60% of rated
   - Monitor temperature: should not exceed 50°C

6. **Charge-Discharge Cycling**
   - Cycle 3-10 times at increasing rates
   - Cycle 1-3: 0.05C
   - Cycle 4-5: 0.1C
   - Cycle 6-7: 0.2C
   - Cycle 8-10: 0.5C
   - Monitor capacity increase toward rated value
   - By cycle 10, should reach 85-95% of rated capacity

### Step 5.2: Performance Testing

| Test Parameter | Target Specification | Method |
|---|---|---|
| **Rated Capacity** | ≥ 95% of nominal | Full charge/discharge cycle |
| **Charge Voltage** | 4.0 ± 0.1 V (typical) | Potentiostatic charging |
| **Discharge Voltage** | 2.5 ± 0.1 V minimum | Galvanostatic discharge |
| **Energy Density** | 250-500 Wh/kg | Calculated from capacity × voltage |
| **Power Density** | 100-1000 W/kg | Discharge at 10C |
| **Cycle Life** | ≥ 500 cycles @ 80% | Repeated formation cycles |
| **Calendar Life** | ≥ 80% capacity after 1 year @ 25°C | Shelf storage test |
| **Interface Resistance** | < 100 Ω·cm² | Electrochemical Impedance Spectroscopy |

### Step 5.3: Safety Testing

1. **Thermal Abuse**
   - Heat cell externally to 150°C
   - Monitor voltage and temperature
   - Should not venting or rupture
   - Solid electrolyte should act as thermal barrier

2. **Mechanical Abuse** (crush test)
   - Apply controlled pressure 50-1000 N
   - Monitor for venting or thermal runaway
   - Solid electrolyte prevents immediate short

3. **Electrical Abuse**
   - Overcharge test: charge to 120% of rated voltage
   - Should tolerate without venting
   - Internal pressure release at design limit

---

## Phase 6: Quality Control & Characterization

### Step 6.1: Non-Destructive Testing

| Test | Equipment | Specification | Frequency |
|------|-----------|---------------|-----------|
| **Electrochemical Impedance** | EIS Analyzer | < 200 Ω @ 1 kHz | Every 5 cycles |
| **Open Circuit Voltage** | Multimeter | ± 10 mV from nominal | Daily |
| **Weight** | Analytical balance | ± 5% of target | 100% |
| **Dimensions** | Caliper | ± 2 mm | 10% sample |
| **Pressure (if applicable)** | Pressure transducer | ± 5% of target | Continuous |

### Step 6.2: Destructive Testing (on sample cells)

1. **Structural Analysis**
   - Dissect cell post-testing
   - Optical microscopy: layer integrity
   - SEM analysis: interfacial contact
   - Electron dispersive X-ray (EDX): elemental mapping

2. **Electrochemical Analysis**
   - Cyclic voltammetry: oxidation/reduction potentials
   - Electrochemical Quartz Crystal Microbalance (EQCM): mass changes
   - Differential pulse voltammetry: trace impurities

---

## Key Differences vs. Lithium-Ion Batteries

| Aspect | Lithium-Ion | Solid-State |
|--------|-------------|-----------|
| **Electrolyte** | Liquid organic | Solid polymer/ceramic |
| **Operating Pressure** | Minimal | 10-100 MPa required |
| **Operating Temperature** | 20-45°C | Often 50-100°C for polymer types |
| **Formation Time** | 10-50 hours | 24-72 hours (slower) |
| **Initial Capacity** | Rapid rise | Gradual rise, longer conditioning |
| **Safety** | Thermal runaway risk | Lower risk (solid electrolyte = barrier) |
| **Manufacturing Complexity** | Moderate | High (requires dry glove box, pressure) |
| **Cost** | Lower | Higher (mature technology) |

---

## Troubleshooting Guide

| Problem | Likely Cause | Solution |
|---------|-------------|----------|
| **No voltage after formation** | Electrolyte not in contact | Increase pressure, re-heat |
| | Electrolyte degradation | Verify electrolyte quality |
| **Very high impedance** | Poor interfacial contact | Increase pressure, thermal cycling |
| | Electrolyte too thin | Verify thickness measurement |
| **Capacity fade after 10 cycles** | Lithium dendrite formation | Reduce current rate, increase pressure |
| | Lithium-electrolyte interface issues | Modify interface coating |
| **Pressure loss during cycling** | Seal leakage | Inspect pouch, re-seal if needed |
| | Material creep | Verify material purity |
| **Low cycle life** | Cathode degradation | Verify cathode chemistry |
| | Electrolyte decomposition | Check temperature control during cycling |

---

## Safety Procedures

- **Lithium Metal Handling**: Always in argon/nitrogen glove box, < 1 ppm O₂ and H₂O
- **Electrolyte Handling**: Polymer/ceramic non-flammable, but keep away from moisture
- **Pressure System**: Inspect regularly for leaks, maintain pressure within design limits
- **Emergency**: Have suitable fire extinguisher (Class C or D for lithium fires)
- **Medical**: Skin contact with lithium → flush with mineral oil, then water (not direct water)

---

## Environmental Conditions

- **Temperature**: 20-25°C for assembly, 50-100°C for conditioning
- **Humidity**: < 1% RH (glove box mandatory)
- **Pressure Environment**: 10-100 MPa maintained throughout device lifetime
- **Inert Atmosphere**: Argon or nitrogen, purity > 99.99%

---

## Personnel Training Requirements

- **Minimum training**: 60 hours (more complex than Li-ion)
- **Dry glove box operation**: 20 hours
- **Lithium metal safety**: 10 hours
- **Equipment operation**: 15 hours
- **Quality procedures**: 10 hours
- **Certification**: Lithium battery safety, solids handling, high-pressure equipment

---

Last Updated: May 14, 2026
Version: 1.0
