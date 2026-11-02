package io.coolbox.synthetic;

import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;

/** Deterministic synthetic data generators (Python: {@code synthetic_data}). */
public final class SyntheticData {
    private SyntheticData() { }
    public static SyntheticDataset makeRegression() { return makeRegression(100,1,0,0,0); }
    public static SyntheticDataset makeRegression(int n) { return makeRegression(n,1,0,0,0); }
    public static SyntheticDataset makeRegression(int n,int features) { return makeRegression(n,features,0,0,0); }
    public static SyntheticDataset makeRegression(int n,int features,double noise) { return makeRegression(n,features,noise,0,0); }
    public static SyntheticDataset makeRegression(int n,int features,double noise,double bias) { return makeRegression(n,features,noise,bias,0); }
    public static SyntheticDataset makeRegression(int n,int features,double noise,double bias,long seed) { int len=checked(n,features); double[] x=new double[len]; double[] y=new double[n]; ErrorBuffer e=new ErrorBuffer(); e.check(SyntheticNative.INSTANCE.coolbox_misc_make_regression(n,features,noise,bias,seed,x,len,y,n,e.pointer(),e.capacity())); return new SyntheticDataset(Matrices.unflatten(x,n,features),y,null); }
    public static SyntheticDataset makeClassification() { return makeClassification(100,2,2,2,0); }
    public static SyntheticDataset makeClassification(int n) { return makeClassification(n,2,2,2,0); }
    public static SyntheticDataset makeClassification(int n,int features) { return makeClassification(n,features,2,2,0); }
    public static SyntheticDataset makeClassification(int n,int features,int classes) { return makeClassification(n,features,classes,2,0); }
    public static SyntheticDataset makeClassification(int n,int features,int classes,double sep) { return makeClassification(n,features,classes,sep,0); }
    public static SyntheticDataset makeClassification(int n,int features,int classes,double sep,long seed) { int len=checked(n,features); double[] x=new double[len]; long[] labels=new long[n]; ErrorBuffer e=new ErrorBuffer(); e.check(SyntheticNative.INSTANCE.coolbox_misc_make_classification(n,features,classes,sep,seed,x,len,labels,n,e.pointer(),e.capacity())); return new SyntheticDataset(Matrices.unflatten(x,n,features),null,labels); }
    public static SyntheticDataset makeBlobs() { return makeBlobs(100,3,2,1,0); }
    public static SyntheticDataset makeBlobs(int n) { return makeBlobs(n,3,2,1,0); }
    public static SyntheticDataset makeBlobs(int n,int centers) { return makeBlobs(n,centers,2,1,0); }
    public static SyntheticDataset makeBlobs(int n,int centers,int features) { return makeBlobs(n,centers,features,1,0); }
    public static SyntheticDataset makeBlobs(int n,int centers,int features,double std) { return makeBlobs(n,centers,features,std,0); }
    public static SyntheticDataset makeBlobs(int n,int centers,int features,double std,long seed) { int len=checked(n,features); double[] x=new double[len]; long[] labels=new long[n]; ErrorBuffer e=new ErrorBuffer(); e.check(SyntheticNative.INSTANCE.coolbox_misc_make_blobs(n,centers,features,std,seed,x,len,labels,n,e.pointer(),e.capacity())); return new SyntheticDataset(Matrices.unflatten(x,n,features),null,labels); }
    private static int checked(int n,int f) { if(n<=0||f<=0) throw new IllegalArgumentException("sample and feature counts must be positive"); return Math.multiplyExact(n,f); }
}
