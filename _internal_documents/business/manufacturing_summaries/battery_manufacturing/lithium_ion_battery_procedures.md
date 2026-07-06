# Lithium-Ion Battery Manufacturing Procedures

## Overview
This document describes the step-by-step procedures for manufacturing lithium-ion batteries from raw materials including cathodes, anodes, copper foil, electrolyte solutions, separators, and packaging materials.

## Required Materials Checklist

### Electrode Materials
- [ ] Cathode material (e.g., LiCoO₂, LiFePO₄, NMC, NCA)
- [ ] Anode material (graphite, silicon composite)
- [ ] Current collectors (aluminum foil for cathode, copper foil for anode)
- [ ] Binders (PVDF, SBR/CMC for aqueous processing)
- [ ] Conductive additives (carbon black, graphene)

### Electrolyte Components
- [ ] Lithium salt (LiPF₆, LiClO₄, LiBF₄)
- [ ] Organic solvents (dimethyl carbonate, ethylene carbonate, diethyl carbonate)
- [ ] Additives (SEI film formers, thermal stabilizers)

### Assembly Materials
- [ ] Separator membrane (microporous polyethylene or polypropylene)
- [ ] Stainless steel or aluminum casing
- [ ] Safety vent
- [ ] Positive/negative terminals
- [ ] Thermal fuses
- [ ] Sealing gaskets
- [ ] Insulating tape

### Quality Control Materials
- [ ] Reference electrodes for potential measurement
- [ ] Calibration standards
- [ ] Testing consumables

---

## Phase 1: Electrode Preparation

### Step 1.1: Cathode Preparation

**Materials:**
- Cathode powder (LiCoO₂, NMC, etc.)
- Binder (PVDF 10-15 wt%)
- Conductive additive (carbon black 2-5 wt%)
- Solvent (N-methylpyrrolidone - NMP)

**Procedure:**

1. **Material Weighing**
   - Weigh cathode powder: target mass × 0.82-0.88 (balance for binder/additive)
   - Weigh binder: cathode mass × 0.12 (10-12% by weight)
   - Weigh conductive additive: cathode mass × 0.04 (3-4% by weight)
   - Record all weights in batch log

2. **Slurry Preparation**
   - Add NMP solvent to mixing vessel (25-30% of final slurry mass)
   - Add PVDF binder gradually while stirring at 300-500 RPM
   - Mix for 10-15 minutes until binder fully dissolved
   - Add conductive additive (carbon black) to binder solution
   - Mix at 500 RPM for 5 minutes
   - Gradually add cathode powder in portions
   - Increase speed to 800-1000 RPM
   - Continue mixing for 30-45 minutes until homogeneous
   - Allow slurry to degas under vacuum (0.1 mbar) for 15-20 minutes

3. **Slurry Quality Check**
   - Visual inspection: uniform color, no lumps
   - Viscosity measurement: 2000-5000 cP (target 3000 cP)
   - Solid content: 45-55% by weight
   - Record results in batch log

### Step 1.2: Anode Preparation

**Materials:**
- Anode powder (graphite or silicon composite)
- Binder (CMC 1-2% + SBR 1-2% for aqueous, or PVDF for NMP)
- Conductive additive (carbon black 1-3 wt%)
- Solvent (water for aqueous, NMP for organic)

**Procedure:**

1. **Material Weighing**
   - Weigh anode powder: target mass × 0.96-0.97
   - Weigh binder: anode mass × 0.02-0.03
   - Weigh conductive additive: anode mass × 0.01-0.02
   - Record weights in batch log

2. **Slurry Preparation (Aqueous Method - preferred)**
   - Add water to vessel (dilute to 50-55% solids)
   - Dissolve CMC (sodium carboxymethyl cellulose) first
   - Add SBR (styrene-butadiene rubber) emulsion
   - Mix at 300 RPM for 10 minutes
   - Add conductive additive
   - Mix for 5 minutes
   - Add anode powder gradually
   - Increase to 800 RPM, mix for 30-40 minutes
   - Check for uniformity
   - Degas under vacuum for 10-15 minutes

3. **Slurry Quality Check**
   - Viscosity: 1000-3000 cP (target 1500 cP)
   - Solid content: 48-52%
   - No settling after 30 minutes standing
   - Record in batch log

---

## Phase 2: Electrode Coating

### Step 2.1: Current Collector Preparation

**For Cathode (Aluminum Foil):**

1. **Surface Cleaning**
   - Inspect aluminum foil for defects, tears, or contamination
   - Mark any defects with position log
   - Clean surface with ethanol wipes if contaminated
   - Air dry completely (no moisture)
   - Check surface resistance: < 0.1 Ω

2. **Dimensional Check**
   - Measure thickness: 10-20 μm (typically 12 μm)
   - Measure width: target ± 2 mm
   - Record dimensions for each batch

**For Anode (Copper Foil):**

1. **Surface Cleaning**
   - Inspect copper foil for oxidation or contamination
   - If oxidized, dip in 1M HCl solution for 30-60 seconds
   - Rinse with deionized water
   - Rinse with ethanol
   - Air dry completely
   - Check surface resistance: < 0.05 Ω

2. **Dimensional Check**
   - Measure thickness: 7-10 μm (typically 8-9 μm)
   - Measure width: target ± 2 mm
   - Record in batch log

### Step 2.2: Electrode Coating Process

**Equipment Setup:**
- Coating machine (doctor blade coater or slot die coater)
- Web tension control system
- Drying oven (50-120°C)
- Thickness gauge

**Coating Parameters:**
- Target coating thickness: 
  - Cathode: 80-120 μm (dry)
  - Anode: 50-80 μm (dry)
- Coating speed: 1-5 m/min
- Doctor blade gap: varies by target thickness

**Procedure:**

1. **Machine Calibration**
   - Load current collector (Al or Cu foil) onto machine
   - Set tension: 50-150 N (based on foil width)
   - Adjust coating head gap to target thickness
   - Run test sheet (scrap material)
   - Measure wet and dry thickness
   - Adjust gap if needed

2. **Coating Application**
   - Fill coating hopper with prepared slurry
   - Check slurry temperature: maintain 20-25°C
   - Start coating machine at low speed (0.5 m/min)
   - Gradually increase to target speed over 2-3 minutes
   - Coat continuously for batch length
   - Monitor wet thickness every 5 meters
   - Record coating parameters every 10 meters

3. **Drying Process**
   - Transport coated foil to drying oven immediately
   - Temperature profile:
     - Zone 1: 60°C for 3-5 minutes
     - Zone 2: 90°C for 5-10 minutes
     - Zone 3: 120°C for 5-10 minutes
   - Belt speed: 1-2 m/min
   - Monitor for blistering or cracking
   - Final moisture content: < 200 ppm (for organic solvents)

4. **Post-Drying Inspection**
   - Allow coated electrode to cool to room temperature
   - Measure dry thickness: ± 10 μm tolerance
   - Visual inspection: uniform coating, no cracks
   - Weight per unit area: record for each batch
   - Electrical resistance check: < 10 Ω for cathode, < 5 Ω for anode

5. **Calender Process** (optional but recommended)
   - Pass dried electrode through pressure calender
   - Pressure: 100-200 bar
   - Temperature: 50-80°C
   - Purpose: improve density and contact
   - Final porosity: 40-60%
   - Thickness increase: 5-15%

---

## Phase 3: Separator Preparation

### Step 3.1: Separator Selection & Processing

**Materials:**
- Microporous separator (polyethylene or polypropylene)
- Thickness: 20-25 μm for commercial cells
- Porosity: 40-60%
- Pore size: 0.3-0.5 μm

**Procedure:**

1. **Separator Inspection**
   - Unroll separator from storage
   - Visual inspection: no holes, tears, or dust
   - Measure thickness: 20-25 μm ± 2 μm
   - Check porosity: should show even translucency
   - Record batch number and supplier

2. **Separator Conditioning**
   - Condition separator at 23°C ± 2°C, 45-55% RH for 24 hours
   - Store in dry, dust-free environment before use
   - Protect from light exposure

3. **Dimensional Preparation**
   - Cut separator to match electrode dimensions
   - Typically 2-3 mm larger than electrode perimeter
   - Cut with clean, sharp blade
   - Remove edge burrs with dry cloth
   - Stack separator sheets between silicone-coated paper

---

## Phase 4: Electrolyte Preparation

### Step 4.1: Electrolyte Solution Formulation

**Standard Formulation:**

1. **Component Preparation**
   
   **For 1L of standard LiPF₆ electrolyte in EC:DEC:**
   
   - LiPF₆ salt: 100 g (1 M)
   - Ethylene carbonate (EC): 300 mL
   - Diethyl carbonate (DEC): 700 mL
   - Additives (optional):
     - Vinylene carbonate: 2-5% vol
     - Lithium difluorophosphate: 1-2% vol

2. **Material Verification**
   - Check LiPF₆ purity: > 99.5%
   - Check EC/DEC purity: > 99% for each component
   - Verify moisture content: < 50 ppm (use Karl Fischer titration)
   - Check expiration dates (lithium salts degradation)

### Step 4.2: Electrolyte Mixing Procedure

**Equipment:**
- Dry glove box (< 1 ppm H₂O, < 1 ppm O₂)
- Precision balance (0.01 g accuracy)
- Magnetic stirrer
- Glass solvent bottle with Teflon-lined cap
- Drying oven (120°C)

**Safety Requirements:**
- All operations in inert atmosphere (argon or nitrogen)
- Use appropriate PPE: nitrile gloves, lab coat, face shield
- Ventilation must be operating
- Have fire extinguisher available (Class D for lithium fires)

**Procedure:**

1. **Glove Box Preparation**
   - Ensure glove box is evacuated and filled with dry inert gas
   - Humidity indicator shows < 1 ppm H₂O
   - Insert all containers to be used through antechamber
   - Allow 30 minutes for pressure equalization

2. **Component Mixing**
   - Weigh LiPF₆ salt: 100 g (±0.5 g)
   - Measure EC: 300 mL (±5 mL)
   - Measure DEC: 700 mL (±5 mL)
   - Combine EC and DEC in bottle first
   - Add LiPF₆ salt gradually with stirring
   - Stir at 500-800 RPM for 60-90 minutes at room temperature
   - Solution may warm slightly (exothermic dissolution)
   - Cool to 20-25°C before continuing

3. **Additive Incorporation** (if used)
   - Allow base electrolyte to cool
   - Add vinylene carbonate: 20-50 mL per liter
   - Add other additives by volume
   - Stir at 300 RPM for 30 minutes
   - Ensure complete mixing

4. **Electrolyte Characterization**
   - Measure ionic conductivity: 8-12 mS/cm (at 25°C)
   - Measure viscosity: 1.0-2.0 cP (typical ~1.2 cP)
   - Measure density: 1.19-1.22 g/cm³
   - Verify pH < 4.0 (for safety)
   - Allow moisture equilibration for 24 hours before final QC

5. **Storage**
   - Transfer to air-tight bottle with Teflon-lined cap
   - Store in dry area at 15-25°C
   - Protect from light
   - Usable for 6-12 months
   - Check ionic conductivity before use if stored > 3 months

---

## Phase 5: Cell Assembly

### Step 5.1: Preparation of Assembly Area

**Environment Control:**
- Temperature: 20-25°C ± 2°C
- Humidity: 40-60% RH
- Cleanliness: ISO Class 6-7 (optional but recommended for high-volume production)

**Equipment:**
- Dry box or glove box for wet assembly (highly recommended)
- Cell case (stainless steel or aluminum, volume = target capacity)
- Positive terminal (stainless steel or nickel-plated)
- Negative terminal (nickel-plated copper)
- Vent
- Gasket (stainless steel with elastomer lining)
- Thermal fuse
- Welding/crimping equipment
- Torque wrench

**Pre-Assembly Checklist:**
- [ ] All materials dried (store at < 2% RH if possible)
- [ ] Electrolyte ionic conductivity verified: 8-12 mS/cm
- [ ] Cell case cleaned and dried
- [ ] Assembly tools sterilized/cleaned
- [ ] Humidity monitor active in assembly area

### Step 5.2: Electrode Assembly (Layer Stack Construction)

**For Cylindrical Cell (18650 example):**

**Stack Order (from negative to positive):**

1. **Negative Current Collector Preparation**
   - Copper foil (anode): strip length ~55 mm
   - Bend into spiral or accordion fold
   - Connect to negative terminal pad
   - Spot weld at 2-3 points
   - Inspect weld quality: no cracks

2. **Layer Construction**
   - Wrap anode sheet around negative current collector spiral
   - Place separator sheet around anode
   - Place cathode sheet inside separator
   - Wrap cathode with second separator sheet (prevent cathode shorting)
   - Connect cathode sheet to positive terminal pad

3. **Electrode Assembly Specs**

| Component | Qty | Specification |
|-----------|-----|---------------|
| Anode coated Cu foil | 1 | 50-80 μm coating, spiral wrapped |
| Separator (PE/PP) | 2-3 | 20-25 μm, total 50-75 μm |
| Cathode coated Al foil | 1 | 80-120 μm coating |
| Negative terminal | 1 | Welded to Cu foil |
| Positive terminal | 1 | Welded to Al foil |

4. **Jelly Roll Assembly** (alternative to layer stack)
   - Pre-cut anode to 100 × 150 mm
   - Pre-cut separator to 102 × 152 mm
   - Pre-cut cathode to 100 × 150 mm
   - Stack: anode → separator → cathode → separator
   - Roll tightly into "jelly roll" form
   - Wrap with insulating tape

### Step 5.3: Insertion into Cell Case

**Procedure:**

1. **Case Preparation**
   - Clean cell case interior with ethanol
   - Dry completely with heat gun (< 50°C)
   - Inspect for internal defects
   - Check internal volume

2. **Assembly Layer by Layer**
   - Insert negative tab through cell bottom insulator
   - Insert electrode assembly (jelly roll or layer stack)
   - Ensure tabs don't touch opposite terminals
   - Insert positive terminal contact plate
   - Verify no physical contact between terminals inside cell

3. **Safety Component Installation**
   - Insert vent (usually pre-installed in cap)
   - Install thermal fuse (if separate component)
   - Verify thermal fuse triggers at 60-80°C

4. **Sealing**
   - Place gasket ring on cell case neck
   - Insert sealing cap
   - Hand crimp or use automatic crimper to seal
   - Torque: 2-5 N·m (depending on design)
   - Verify seal integrity: no electrolyte leakage

---

## Phase 6: Electrolyte Filling

### Step 6.1: Electrolyte Addition

**Procedure:**

1. **Electrolyte Preparation**
   - Verify electrolyte ionic conductivity: 8-12 mS/cm
   - Verify electrolyte moisture: < 200 ppm
   - Warm to 25-30°C if stored cold
   - Measure volume needed: 0.8-1.2 mL per Ah of capacity

2. **Filling Process** (must be done in dry glove box if possible)
   - Drill small hole (1 mm) in cell cap (if not pre-drilled)
   - Use syringe with needle to inject electrolyte
   - Inject slowly: 0.5 mL/minute
   - Monitor internal pressure to avoid buildup
   - For 2500 mAh cell: inject ~2.5 mL total
   - Allow 2-5 minutes for electrolyte to saturate separator

3. **Void Removal**
   - Gently tap cell on soft surface 5-10 times
   - Shake cell horizontally for 1-2 minutes
   - This helps electrolyte distribute and removes air pockets
   - Allow cell to rest for 10-15 minutes

4. **Cap Sealing**
   - Remove syringe
   - Clean excess electrolyte with dry wipe
   - Install permanent cap
   - Crimp or weld cap to cell case
   - Torque spec: 2-5 N·m
   - Verify seal: no electrolyte leakage
   - Allow to set for 10 minutes

---

## Phase 7: Cell Conditioning & Testing

### Step 7.1: Initial Rest Period

1. **Room Temperature Rest**
   - Allow assembled cell to sit at 20-25°C for 24 hours
   - Purpose: electrolyte wetting and ion exchange initialization
   - Store in cool, dry location
   - Monitor for any external leakage

### Step 7.2: Formation Cycling

**Purpose:** Establish solid electrolyte interphase (SEI) layer and activate anode

**Procedure:**

1. **Slow Charge Formation**
   - Initial voltage formation at constant current (CC) mode
   - Charge current: 0.1C (C = rated capacity in Ah)
   - Target voltage: 4.2V (for LiCoO₂)
   - Time: 10-15 hours
   - Monitor temperature: should not exceed 45°C

2. **First Discharge**
   - Discharge at 0.1C to 3.0V (minimum safe voltage)
   - Measure capacity: should be 50-70% of rated
   - Measure internal resistance

3. **Subsequent Cycles** (3-5 cycles typical)
   - Charge at 0.2-0.5C to 4.2V
   - Rest 30 minutes at open circuit
   - Discharge at 0.5C to 3.0V
   - Monitor capacity increase
   - By cycle 3, should reach > 95% rated capacity

### Step 7.3: Performance Testing

| Test | Specification | Pass/Fail |
|------|---------------|-----------|
| **Rated Capacity** | ≥ 95% of nominal | |
| **Voltage (fully charged)** | 4.20 ± 0.05 V | |
| **Voltage (fully discharged)** | 3.00 ± 0.05 V | |
| **Internal Resistance** | < 100 mΩ @ 1kHz | |
| **Self-discharge rate** | < 2% per month @ 20°C | |
| **Cycle life (80% retention)** | ≥ 500 cycles | |

### Step 7.4: Safety Testing

1. **Overcharge Protection Test**
   - Charge to 4.5V (+ 300 mV over nominal)
   - Monitor for venting or thermal runaway
   - Should trigger internal protection

2. **Short Circuit Test**
   - Connect resistor (0.05 Ω) across terminals
   - Monitor temperature and voltage drop
   - Should not exceed 80°C
   - Cell should vent safely if exceeded

3. **Thermal Abuse Test**
   - Heat cell externally to 80°C
   - Verify thermal management system responds
   - Should vent safely at < 100°C

---

## Quality Control Checkpoints

| Phase | Checkpoint | Tolerance | Document |
|-------|-----------|-----------|----------|
| **Electrode Prep** | Slurry viscosity | 3000 ± 500 cP | Batch QC Log |
| | Coating thickness | ± 10 μm | Thickness Report |
| **Separator** | Thickness | 20-25 ± 2 μm | Supplier Cert |
| **Electrolyte** | Ionic conductivity | 8-12 mS/cm | Solution QC Log |
| | Moisture content | < 200 ppm | Karl Fischer Report |
| **Assembly** | Cell weight | ± 5% target | Cell Log |
| | Internal resistance | < 100 mΩ | Resistance Test Report |
| **Testing** | Rated capacity | ≥ 95% nominal | Formation Report |
| | Voltage @ full charge | 4.20 ± 0.05 V | Test Data |

---

## Troubleshooting Guide

| Issue | Cause | Solution |
|-------|-------|----------|
| **Low capacity** | Insufficient electrolyte | Refill before sealing |
| | Poor electrode contact | Check electrode wrapping |
| | High moisture in components | Re-dry under vacuum |
| **High internal resistance** | Thick SEI formation | Extended formation cycles |
| | Poor current collector contact | Check terminal welds |
| **Electrolyte leakage** | Improper seal | Re-crimp with correct torque |
| | Over-pressurization | Use pressure release valve |
| **Thermal runaway** | Separator rupture | Use thicker separator batch |
| | Internal short circuit | Inspect jelly roll alignment |
| **Voltage drop during use** | Anode surface issues | Use fresh copper foil |
| | Poor cathode material | Verify material quality |

---

## Storage & Shelf Life

- **Charged cells**: Store at 20-25°C, 40-60% RH, 50% SOC (state of charge)
- **Shelf life**: 12-24 months from formation
- **Monthly self-discharge**: < 2% at 20°C
- **Temperature sensitivity**: Every 10°C increase reduces shelf life by ~50%

---

## Personnel Requirements & Training

- **Minimum training**: 40 hours hands-on under supervision
- **Required certifications**: 
  - Lithium battery handling safety
  - Electrolyte safety procedures
  - Equipment operation
- **Experience requirement**: Minimum 3 months production support before independent work

---

Last Updated: May 14, 2026
Version: 1.0
