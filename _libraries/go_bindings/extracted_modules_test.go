package coolboxgo

import "testing"

func TestHMMSmoke(t *testing.T) {
	hmm, err := NewHMM(2, 2)
	if err != nil {
		t.Fatalf("new HMM failed: %v", err)
	}
	defer hmm.Close()

	if err := hmm.SetInitialProbabilities([]float64{0.6, 0.4}); err != nil {
		t.Fatalf("set initial probabilities failed: %v", err)
	}
	if err := hmm.SetTransitionMatrix([][]float64{{0.7, 0.3}, {0.4, 0.6}}); err != nil {
		t.Fatalf("set transition matrix failed: %v", err)
	}
	if err := hmm.SetEmissionMatrix([][]float64{{0.5, 0.5}, {0.1, 0.9}}); err != nil {
		t.Fatalf("set emission matrix failed: %v", err)
	}

	initial, err := hmm.InitialProbabilities()
	if err != nil {
		t.Fatalf("get initial probabilities failed: %v", err)
	}
	if len(initial) != 2 {
		t.Fatalf("unexpected initial probability length: %d", len(initial))
	}

	transition, err := hmm.TransitionMatrix()
	if err != nil {
		t.Fatalf("get transition matrix failed: %v", err)
	}
	if len(transition) != 2 || len(transition[0]) != 2 {
		t.Fatalf("unexpected transition shape: %dx%d", len(transition), len(transition[0]))
	}

	emission, err := hmm.EmissionMatrix()
	if err != nil {
		t.Fatalf("get emission matrix failed: %v", err)
	}
	if len(emission) != 2 || len(emission[0]) != 2 {
		t.Fatalf("unexpected emission shape: %dx%d", len(emission), len(emission[0]))
	}

	logLikelihood, err := hmm.LogLikelihood([]int{0, 1, 1, 0})
	if err != nil {
		t.Fatalf("log likelihood failed: %v", err)
	}
	if logLikelihood == 0 {
		t.Fatal("log likelihood should not be zero for the configured model")
	}

	states, err := hmm.MostLikelyStates([]int{0, 1, 1, 0})
	if err != nil {
		t.Fatalf("most likely states failed: %v", err)
	}
	if len(states) != 4 {
		t.Fatalf("unexpected decoded state length: %d", len(states))
	}
}

func TestPCASmoke(t *testing.T) {
	pca, err := NewPCA(1, true, false)
	if err != nil {
		t.Fatalf("new PCA failed: %v", err)
	}
	defer pca.Close()

	x := [][]float64{{1, 2}, {2, 3}, {3, 4}, {4, 5}}
	if err := pca.Fit(x); err != nil {
		t.Fatalf("fit failed: %v", err)
	}
	if !pca.IsFitted() {
		t.Fatal("expected PCA to report fitted state")
	}

	transformed, err := pca.Transform(x)
	if err != nil {
		t.Fatalf("transform failed: %v", err)
	}
	if len(transformed) != len(x) || len(transformed[0]) != 1 {
		t.Fatalf("unexpected transformed shape: %dx%d", len(transformed), len(transformed[0]))
	}

	components, err := pca.Components()
	if err != nil {
		t.Fatalf("components failed: %v", err)
	}
	if len(components) == 0 || len(components[0]) == 0 {
		t.Fatal("components should not be empty")
	}

	variance, err := pca.ExplainedVariance()
	if err != nil {
		t.Fatalf("explained variance failed: %v", err)
	}
	if len(variance) != 1 {
		t.Fatalf("unexpected explained variance length: %d", len(variance))
	}

	mean, err := pca.Mean()
	if err != nil {
		t.Fatalf("mean failed: %v", err)
	}
	if len(mean) != 2 {
		t.Fatalf("unexpected mean length: %d", len(mean))
	}
}

func TestSVMSmoke(t *testing.T) {
	svm, err := NewSVM(KernelLinear, 1.0, 0.0, 0)
	if err != nil {
		t.Fatalf("new SVM failed: %v", err)
	}
	defer svm.Close()

	x := [][]float64{{0, 0}, {0, 1}, {1, 0}, {1, 1}}
	y := []float64{-1, -1, 1, 1}
	if err := svm.Fit(x, y); err != nil {
		t.Fatalf("fit failed: %v", err)
	}

	prediction, err := svm.Predict([]float64{1, 0})
	if err != nil {
		t.Fatalf("predict failed: %v", err)
	}
	if prediction == 0 {
		t.Fatal("prediction should not be zero for the trained sample")
	}
}

func TestBanditSmoke(t *testing.T) {
	arm, err := NewBanditArm(0.7)
	if err != nil {
		t.Fatalf("new bandit arm failed: %v", err)
	}
	defer arm.Close()

	if _, err := arm.Pull(); err != nil {
		t.Fatalf("arm pull failed: %v", err)
	}
	if err := arm.Update(1.0); err != nil {
		t.Fatalf("arm update failed: %v", err)
	}
	if arm.PullCount() < 0 {
		t.Fatalf("unexpected pull count: %d", arm.PullCount())
	}

	agent, err := NewEpsilonGreedyAgent([]float64{0.2, 0.8}, 0.1, 42)
	if err != nil {
		t.Fatalf("new epsilon-greedy agent failed: %v", err)
	}
	defer agent.Close()

	if err := agent.RunSimulation(20); err != nil {
		t.Fatalf("run simulation failed: %v", err)
	}
	results, err := agent.Results()
	if err != nil {
		t.Fatalf("results failed: %v", err)
	}
	if len(results.Bandits) != 2 {
		t.Fatalf("unexpected bandit result count: %d", len(results.Bandits))
	}
}

func TestBayesianNetworkSmoke(t *testing.T) {
	bn := NewBayesianNetwork()
	if bn == nil || bn.handle == nil {
		t.Fatal("failed to create bayesian network")
	}
	defer bn.Close()

	weatherID, err := bn.AddNode("weather", []string{"sunny", "rainy"})
	if err != nil {
		t.Fatalf("add weather node failed: %v", err)
	}
	trafficID, err := bn.AddNode("traffic", []string{"light", "heavy"})
	if err != nil {
		t.Fatalf("add traffic node failed: %v", err)
	}
	if err := bn.AddEdge(weatherID, trafficID); err != nil {
		t.Fatalf("add edge failed: %v", err)
	}
	if err := bn.SetCPT(weatherID, []float64{0.7, 0.3}); err != nil {
		t.Fatalf("set root CPT failed: %v", err)
	}
	if err := bn.SetCPT(trafficID, []float64{0.8, 0.2, 0.3, 0.7}); err != nil {
		t.Fatalf("set child CPT failed: %v", err)
	}

	if got := bn.NumNodes(); got != 2 {
		t.Fatalf("unexpected node count: %d", got)
	}
	lookupID, err := bn.GetNodeID("traffic")
	if err != nil {
		t.Fatalf("get node id failed: %v", err)
	}
	if lookupID != trafficID {
		t.Fatalf("unexpected node id: got %d want %d", lookupID, trafficID)
	}

	stateCount, err := bn.NodeStateCount(trafficID)
	if err != nil {
		t.Fatalf("state count failed: %v", err)
	}
	if stateCount != 2 {
		t.Fatalf("unexpected state count: %d", stateCount)
	}

	distribution, err := bn.Query(trafficID, map[int]int{weatherID: 0})
	if err != nil {
		t.Fatalf("query failed: %v", err)
	}
	if len(distribution) != 2 {
		t.Fatalf("unexpected query result length: %d", len(distribution))
	}
}

func TestGUIConstructorsAndGuards(t *testing.T) {
	if NewToolbar(nil) != nil {
		t.Fatal("expected nil toolbar for empty actions")
	}
	if NewLayerList(nil, 0) != nil {
		t.Fatal("expected nil layer list for empty layers")
	}
	if NewPropertyInspector([]string{"k"}, nil) != nil {
		t.Fatal("expected nil property inspector for mismatched inputs")
	}
	if NewRadioSelector(nil, 0) != nil {
		t.Fatal("expected nil radio selector for empty options")
	}
	if NewCheckboxGroup([]string{"a"}, nil) != nil {
		t.Fatal("expected nil checkbox group for mismatched options and states")
	}

	component := NewComponent(0)
	if component == nil || component.handle == nil {
		t.Fatal("failed to create component")
	}
	toolbar := NewToolbar([]string{"save", "open"})
	if toolbar == nil || toolbar.handle == nil {
		t.Fatal("failed to create toolbar")
	}
	dockPanel := NewDockPanel("main", true)
	if dockPanel == nil || dockPanel.handle == nil {
		t.Fatal("failed to create dock panel")
	}
	layerList := NewLayerList([]string{"a", "b"}, 1)
	if layerList == nil || layerList.handle == nil {
		t.Fatal("failed to create layer list")
	}
	propertyInspector := NewPropertyInspector([]string{"name"}, []string{"value"})
	if propertyInspector == nil || propertyInspector.handle == nil {
		t.Fatal("failed to create property inspector")
	}
	fileTree := NewFileTree("root")
	if fileTree == nil || fileTree.handle == nil {
		t.Fatal("failed to create file tree")
	}
	radioSelector := NewRadioSelector([]string{"left", "right"}, 0)
	if radioSelector == nil || radioSelector.handle == nil {
		t.Fatal("failed to create radio selector")
	}
	checkboxGroup := NewCheckboxGroup([]string{"visible", "locked"}, []bool{true, false})
	if checkboxGroup == nil || checkboxGroup.handle == nil {
		t.Fatal("failed to create checkbox group")
	}
}

func TestGraphicsGuardPaths(t *testing.T) {
	graph := NewGraph(200, 100, 0)
	if graph == nil || graph.handle == nil {
		t.Fatal("failed to create graph")
	}

	if err := graph.AddSeries("empty", nil, nil, NewColor(1, 2, 3, 255)); err == nil {
		t.Fatal("expected empty series to fail")
	}
	if err := graph.AddSeries("nil-color", []float64{1}, []float64{2}, nil); err == nil {
		t.Fatal("expected nil color to fail")
	}

	table := NewTable()
	if table == nil || table.handle == nil {
		t.Fatal("failed to create table")
	}
	table.SetHeaders(nil)
	table.AddRow(nil)
}
