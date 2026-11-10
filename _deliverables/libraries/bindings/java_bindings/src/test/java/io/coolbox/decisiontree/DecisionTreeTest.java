package io.coolbox.decisiontree;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class DecisionTreeTest {@Test void fitsTreeAndForest(){int[][] x={{0},{1},{2},{3}};int[] y={0,0,1,1};try(DecisionTree t=new DecisionTree().fit(x,y,2);RandomForest f=new RandomForest(5,2).fit(x,y)){assertEquals(0,t.predict(new int[]{0}));assertEquals(1,f.predict(new int[]{3}));}}@Test void rejectsRaggedFeatures(){try(DecisionTree t=new DecisionTree()){assertThrows(IllegalArgumentException.class,()->t.fit(new int[][]{{1},{1,2}},new int[]{0,1},2));}}}
