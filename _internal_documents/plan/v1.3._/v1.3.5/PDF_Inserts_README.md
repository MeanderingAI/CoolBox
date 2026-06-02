# PDF Inserts Index

This document lists all PDF files ("pdf inserts") included in this repository, with their relative paths for reference.

---

## Engineering Documentation Slides
- _internal_documents/engineering_documention/slides/CoolBox_AssetAssement.pdf
- _internal_documents/engineering_documention/slides/CodeBaseReview.pdf

## Business Asset Record Budgeting – Prebuilts (TS_P340)
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/rn_common_commercial_smb_dtws.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/quadro-pascal-p620-data-sheet-593981-us-nv-r3-web.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/p340_tiny_hmm_en.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/p340_sff_hmm.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/external_power_supplies_datasheet_dt_ws_2020_jan.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/prebuilts/TS_P340/dt_nb_common_swg_en.pdf

## Business Asset Record Budgeting – Custom Build (quick_can)
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/mb_manual_x399-features.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/mb_manual_x399-designare-ex_sc.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/mb_manual_x399-designare-ex_j.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/mb_manual_x399-designare-ex_e.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/mb_manual_quick-guide_amd-trx.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/GEFORCE_RTX_2080Ti_User_Guide.pdf
- _internal_documents/business/asset_record_budgetting/assets/computers/custom_build/quick_can/454674.pdf

---

## v1.3.5 Technical Update (Python Bindings)

Alongside the PDF insert inventory, the v1.3.5 cycle also includes a completed Python bindings stabilization pass:

1. [python_linear_regression_fit_segfault_fix.md](python_linear_regression_fit_segfault_fix.md)
2. [python_deep_learning_wrapper_include_fix.md](python_deep_learning_wrapper_include_fix.md)
3. [python_hmm_bindings_repair.md](python_hmm_bindings_repair.md)
4. [python_pde_spde_binding_source_inclusion_fix.md](python_pde_spde_binding_source_inclusion_fix.md)

Validation:

1. `test_bindings.py` now passes (`9/9`).
2. `hmm_python_bindings_test` ctest now passes.

## v1.3.5 Technical Update (Build and Test Stabilization)

Additional v1.3.5 stabilization work has been broken out into individual notes:

1. [cp_decomposition_khatri_rao_fix.md](cp_decomposition_khatri_rao_fix.md)
2. [matrix_profile_tolerance_adjustment.md](matrix_profile_tolerance_adjustment.md)
3. [gabor_kernel_unnormalized_center_test_fix.md](gabor_kernel_unnormalized_center_test_fix.md)
4. [radix_sort_msd_stabilization.md](radix_sort_msd_stabilization.md)
5. [eigen_ctest_pollution_prevention.md](eigen_ctest_pollution_prevention.md)

Validation:

1. `ctest --output-on-failure -j 8` passes (`36/36`).
2. `make test` passes with no CTest failures (`36/36`).

_Last updated: June 2, 2026_
