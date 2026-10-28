package io.coolbox.opt;
public record OwaCrossValidationResult(double meanLoss,double[] foldLosses) {}
