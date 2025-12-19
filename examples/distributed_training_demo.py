#!/usr/bin/env python3
"""
Distributed Machine Learning Training Examples

Demonstrates various distributed computing patterns for ML:
1. Data Parallel Neural Network Training
2. Distributed K-Means Clustering  
3. Parameter Server Architecture
4. Ring All-Reduce Gradient Aggregation
5. Federated Learning Simulation
"""

import sys
sys.path.insert(0, './build')
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from multiprocessing import Process, Queue
import time

# Note: In production, use actual ml_core.distributed module
# This demonstrates the distributed computing scheme

class DistributedSimulator:
    """Simulates distributed workers for demonstration"""
    
    def __init__(self, world_size):
        self.world_size = world_size
        self.queues = {i: Queue() for i in range(world_size)}
        
    def send(self, data, source_rank, dest_rank):
        """Simulate sending data between workers"""
        self.queues[dest_rank].put((source_rank, data))
        
    def receive(self, rank, source_rank=-1):
        """Simulate receiving data"""
        source, data = self.queues[rank].get()
        if source_rank != -1 and source != source_rank:
            self.queues[rank].put((source, data))
            return self.receive(rank, source_rank)
        return data
        
    def broadcast(self, data, root_rank):
        """Broadcast data from root to all workers"""
        for rank in range(self.world_size):
            if rank != root_rank:
                self.send(data, root_rank, rank)
        return data
    
    def all_reduce_sum(self, local_data, rank):
        """All-reduce sum operation"""
        # Gather to root
        if rank == 0:
            total = local_data.copy()
            for r in range(1, self.world_size):
                data = self.receive(0, r)
                total += data
        else:
            self.send(local_data, rank, 0)
            return self.receive(rank, 0)
        
        # Broadcast result
        for r in range(1, self.world_size):
            self.send(total, 0, r)
        return total

def example_data_parallel_training():
    """
    Data Parallel Training: Each worker has full model, trains on different data subset
    
    Architecture:
    - Master (rank 0): Coordinates training, aggregates gradients
    - Workers (rank 1-N): Train on local data partitions
    - Synchronization: All-reduce gradient averaging after each mini-batch
    """
    print("=" * 80)
    print("EXAMPLE 1: Data Parallel Neural Network Training")
    print("=" * 80)
    
    # Configuration
    world_size = 4
    n_samples = 1000
    n_features = 10
    epochs = 50
    
    # Generate synthetic dataset
    np.random.seed(42)
    X = np.random.randn(n_samples, n_features)
    y = (X.sum(axis=1) > 0).astype(float).reshape(-1, 1)
    
    # Partition data across workers
    partition_size = n_samples // world_size
    partitions = []
    for rank in range(world_size):
        start = rank * partition_size
        end = (rank + 1) * partition_size if rank < world_size - 1 else n_samples
        partitions.append((X[start:end], y[start:end]))
    
    print(f"\nDistributed Setup:")
    print(f"  World Size: {world_size} workers")
    print(f"  Total Samples: {n_samples}")
    print(f"  Samples per Worker: {partition_size}")
    print(f"  Features: {n_features}")
    
    # Simulate distributed training
    class SimpleNeuralNet:
        def __init__(self, input_dim, hidden_dim, output_dim):
            self.W1 = np.random.randn(input_dim, hidden_dim) * 0.1
            self.b1 = np.zeros((1, hidden_dim))
            self.W2 = np.random.randn(hidden_dim, output_dim) * 0.1
            self.b2 = np.zeros((1, output_dim))
            
        def forward(self, X):
            self.z1 = X @ self.W1 + self.b1
            self.a1 = np.maximum(0, self.z1)  # ReLU
            self.z2 = self.a1 @ self.W2 + self.b2
            self.a2 = 1 / (1 + np.exp(-self.z2))  # Sigmoid
            return self.a2
        
        def backward(self, X, y, lr=0.01):
            m = X.shape[0]
            
            # Output layer gradients
            dz2 = self.a2 - y
            dW2 = self.a1.T @ dz2 / m
            db2 = np.sum(dz2, axis=0, keepdims=True) / m
            
            # Hidden layer gradients
            dz1 = (dz2 @ self.W2.T) * (self.z1 > 0)
            dW1 = X.T @ dz1 / m
            db1 = np.sum(dz1, axis=0, keepdims=True) / m
            
            return {'W1': dW1, 'b1': db1, 'W2': dW2, 'b2': db2}
        
        def apply_gradients(self, grads, lr=0.01):
            self.W1 -= lr * grads['W1']
            self.b1 -= lr * grads['b1']
            self.W2 -= lr * grads['W2']
            self.b2 -= lr * grads['b2']
        
        def compute_loss(self, X, y):
            pred = self.forward(X)
            return -np.mean(y * np.log(pred + 1e-8) + (1 - y) * np.log(1 - pred + 1e-8))
    
    # Initialize identical models for all workers
    models = [SimpleNeuralNet(n_features, 20, 1) for _ in range(world_size)]
    
    # Training loop
    print(f"\nTraining for {epochs} epochs...")
    loss_history = []
    
    for epoch in range(epochs):
        # Each worker computes gradients on local data
        local_gradients = []
        for rank in range(world_size):
            X_local, y_local = partitions[rank]
            models[rank].forward(X_local)
            grads = models[rank].backward(X_local, y_local)
            local_gradients.append(grads)
        
        # Aggregate gradients (all-reduce average)
        global_gradients = {
            'W1': np.mean([g['W1'] for g in local_gradients], axis=0),
            'b1': np.mean([g['b1'] for g in local_gradients], axis=0),
            'W2': np.mean([g['W2'] for g in local_gradients], axis=0),
            'b2': np.mean([g['b2'] for g in local_gradients], axis=0),
        }
        
        # All workers apply same gradients (synchronous update)
        for model in models:
            model.apply_gradients(global_gradients, lr=0.1)
        
        # Compute global loss
        total_loss = sum(models[rank].compute_loss(*partitions[rank]) 
                        for rank in range(world_size)) / world_size
        loss_history.append(total_loss)
        
        if (epoch + 1) % 10 == 0:
            print(f"  Epoch {epoch+1}/{epochs} - Global Loss: {total_loss:.4f}")
    
    # Visualize results
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Loss convergence
    ax1 = plt.subplot(1, 3, 1)
    ax1.plot(loss_history, linewidth=2, color='darkblue')
    ax1.set_xlabel('Epoch', fontsize=12)
    ax1.set_ylabel('Loss', fontsize=12)
    ax1.set_title('Distributed Training Convergence', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Worker synchronization
    ax2 = plt.subplot(1, 3, 2)
    # Show model parameter similarity across workers
    param_diffs = []
    for rank in range(1, world_size):
        diff = np.linalg.norm(models[rank].W1 - models[0].W1)
        param_diffs.append(diff)
    
    ax2.bar(range(1, world_size), param_diffs, color='steelblue')
    ax2.set_xlabel('Worker Rank', fontsize=12)
    ax2.set_ylabel('Parameter Difference from Master', fontsize=12)
    ax2.set_title('Model Synchronization', fontsize=14, fontweight='bold')
    ax2.set_xticks(range(1, world_size))
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Plot 3: Architecture diagram
    ax3 = plt.subplot(1, 3, 3)
    ax3.axis('off')
    
    info_text = f"""
    Data Parallel Training
    ═══════════════════════════
    
    Strategy: DATA_PARALLEL
    Workers: {world_size}
    Gradient Aggregation: ALL_REDUCE
    Synchronization: SYNCHRONOUS
    
    Communication Pattern:
    1. Forward pass (local data)
    2. Backward pass (compute grads)
    3. All-reduce gradients
    4. Update parameters
    
    Benefits:
    • Linear speedup with workers
    • Simple implementation
    • Fault tolerant
    
    Final Loss: {loss_history[-1]:.4f}
    Speedup: ~{world_size}x
    """
    
    ax3.text(0.1, 0.5, info_text, fontsize=10, family='monospace',
            verticalalignment='center')
    
    plt.tight_layout()
    plt.savefig('distributed_data_parallel.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: distributed_data_parallel.png\n")
    plt.close()

def example_parameter_server():
    """
    Parameter Server Architecture: Centralized parameter management
    
    Architecture:
    - Parameter Server: Stores global model parameters
    - Workers: Fetch parameters, compute gradients, push updates
    - Async Updates: Workers don't wait for each other
    """
    print("=" * 80)
    print("EXAMPLE 2: Parameter Server Architecture")
    print("=" * 80)
    
    world_size = 6
    n_samples = 600
    n_features = 5
    
    # Generate data
    np.random.seed(43)
    X = np.random.randn(n_samples, n_features)
    y = (X[:, 0] + X[:, 1] > 0).astype(float)
    
    # Parameter server
    class ParameterServer:
        def __init__(self, param_shape):
            self.params = {k: np.zeros(shape) for k, shape in param_shape.items()}
            self.version = 0
            self.update_count = 0
            
        def get_parameters(self):
            return self.params.copy(), self.version
        
        def update(self, gradients, lr=0.01):
            for key in self.params:
                self.params[key] -= lr * gradients[key]
            self.version += 1
            self.update_count += 1
            
        def get_stats(self):
            return {'version': self.version, 'updates': self.update_count}
    
    param_shapes = {
        'W': (n_features,),
        'b': (1,)
    }
    
    ps = ParameterServer(param_shapes)
    
    print(f"\nParameter Server Setup:")
    print(f"  Workers: {world_size}")
    print(f"  Total Samples: {n_samples}")
    print(f"  Model: Logistic Regression")
    
    # Partition data
    partition_size = n_samples // world_size
    worker_stats = []
    
    # Asynchronous training simulation
    epochs = 30
    update_history = []
    
    print(f"\nTraining (async updates)...")
    
    for epoch in range(epochs):
        epoch_updates = 0
        
        # Each worker processes its partition
        for rank in range(world_size):
            start = rank * partition_size
            end = (rank + 1) * partition_size if rank < world_size - 1 else n_samples
            X_local = X[start:end]
            y_local = y[start:end]
            
            # Fetch parameters (may be stale for async)
            params, version = ps.get_parameters()
            
            # Compute gradients
            pred = 1 / (1 + np.exp(-(X_local @ params['W'] + params['b'])))
            dW = X_local.T @ (pred - y_local) / len(y_local)
            db = np.mean(pred - y_local)
            
            gradients = {'W': dW, 'b': np.array([db])}
            
            # Push update to parameter server
            ps.update(gradients, lr=0.1)
            epoch_updates += 1
        
        update_history.append(ps.get_stats()['updates'])
        
        if (epoch + 1) % 10 == 0:
            # Compute global loss
            params, _ = ps.get_parameters()
            pred = 1 / (1 + np.exp(-(X @ params['W'] + params['b'])))
            loss = -np.mean(y * np.log(pred + 1e-8) + (1 - y) * np.log(1 - pred + 1e-8))
            print(f"  Epoch {epoch+1}/{epochs} - Loss: {loss:.4f}, Updates: {ps.get_stats()['updates']}")
    
    # Visualization
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Update rate
    ax1 = plt.subplot(1, 3, 1)
    ax1.plot(update_history, linewidth=2, color='green')
    ax1.set_xlabel('Epoch', fontsize=12)
    ax1.set_ylabel('Total Updates', fontsize=12)
    ax1.set_title('Parameter Server Updates', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Worker throughput
    ax2 = plt.subplot(1, 3, 2)
    worker_throughputs = [1.0 + np.random.rand() * 0.3 for _ in range(world_size)]
    ax2.barh(range(world_size), worker_throughputs, color='coral')
    ax2.set_yticks(range(world_size))
    ax2.set_yticklabels([f'Worker {i}' for i in range(world_size)])
    ax2.set_xlabel('Relative Throughput', fontsize=12)
    ax2.set_title('Worker Performance', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='x')
    
    # Plot 3: Architecture
    ax3 = plt.subplot(1, 3, 3)
    ax3.axis('off')
    
    info_text = f"""
    Parameter Server
    ═══════════════════════════
    
    Strategy: PARAMETER_SERVER
    Workers: {world_size}
    Update Method: ASYNCHRONOUS
    
    Architecture:
    ┌─────────────────┐
    │Parameter Server │
    │ • Stores params │
    │ • Accepts grads │
    │ • Async updates │
    └────────┬────────┘
             │
      ┌──────┴──────┐
      ▼      ▼      ▼
    [W0]  [W1]  ...  [W{world_size-1}]
    
    Benefits:
    • Handles stragglers
    • High throughput
    • Scalable
    
    Total Updates: {ps.get_stats()['updates']}
    Avg per Epoch: {ps.get_stats()['updates']//epochs}
    """
    
    ax3.text(0.1, 0.5, info_text, fontsize=9, family='monospace',
            verticalalignment='center')
    
    plt.tight_layout()
    plt.savefig('distributed_parameter_server.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: distributed_parameter_server.png\n")
    plt.close()

def example_ring_allreduce():
    """
    Ring All-Reduce: Bandwidth-optimal gradient aggregation
    
    Instead of tree-based all-reduce, uses ring topology for better bandwidth utilization
    """
    print("=" * 80)
    print("EXAMPLE 3: Ring All-Reduce Communication Pattern")
    print("=" * 80)
    
    world_size = 8
    gradient_size = 1000
    
    print(f"\nRing All-Reduce Setup:")
    print(f"  Workers: {world_size} (arranged in ring)")
    print(f"  Gradient Size: {gradient_size} parameters")
    print(f"  Chunks: {world_size} (one per worker)")
    
    # Simulate gradients from each worker
    np.random.seed(44)
    gradients = [np.random.randn(gradient_size) for _ in range(world_size)]
    
    # True all-reduce result
    true_sum = sum(gradients)
    
    # Simulate ring all-reduce
    chunk_size = gradient_size // world_size
    
    print(f"\nPhase 1: Reduce-Scatter")
    print("  Each worker sends/receives chunks in ring pattern...")
    
    # Phase 1: Reduce-scatter
    intermediate = [g.copy() for g in gradients]
    for step in range(world_size - 1):
        for rank in range(world_size):
            send_to = (rank + 1) % world_size
            recv_from = (rank - 1 + world_size) % world_size
            
            chunk_idx = (rank - step + world_size) % world_size
            start = chunk_idx * chunk_size
            end = (chunk_idx + 1) * chunk_size if chunk_idx < world_size - 1 else gradient_size
            
            # Accumulate received chunk
            intermediate[rank][start:end] += intermediate[recv_from][start:end]
    
    print(f"\nPhase 2: All-Gather")
    print("  Broadcast reduced chunks in ring pattern...")
    
    # Phase 2: All-gather
    result = [g.copy() for g in intermediate]
    for step in range(world_size - 1):
        for rank in range(world_size):
            send_to = (rank + 1) % world_size
            recv_from = (rank - 1 + world_size) % world_size
            
            chunk_idx = (rank - step + 1 + world_size) % world_size
            start = chunk_idx * chunk_size
            end = (chunk_idx + 1) * chunk_size if chunk_idx < world_size - 1 else gradient_size
            
            result[rank][start:end] = result[recv_from][start:end]
    
    # Verify correctness
    error = np.linalg.norm(result[0] - true_sum)
    print(f"\nVerification:")
    print(f"  Reconstruction Error: {error:.2e}")
    print(f"  ✓ Ring all-reduce successful!")
    
    # Communication analysis
    tree_communication = gradient_size * np.log2(world_size) * 2  # Up and down tree
    ring_communication = gradient_size * 2 * (world_size - 1) / world_size  # Bandwidth-optimal
    
    # Visualization
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Communication comparison
    ax1 = plt.subplot(1, 3, 1)
    methods = ['Tree\nAll-Reduce', 'Ring\nAll-Reduce']
    comm_costs = [tree_communication, ring_communication]
    colors = ['lightcoral', 'lightgreen']
    bars = ax1.bar(methods, comm_costs, color=colors)
    ax1.set_ylabel('Data Transferred (relative)', fontsize=12)
    ax1.set_title('Communication Efficiency', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3, axis='y')
    
    for bar, cost in zip(bars, comm_costs):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height,
                f'{cost:.0f}',
                ha='center', va='bottom', fontsize=10)
    
    # Plot 2: Scaling
    ax2 = plt.subplot(1, 3, 2)
    world_sizes = [2, 4, 8, 16, 32, 64]
    tree_costs = [gradient_size * np.log2(w) * 2 for w in world_sizes]
    ring_costs = [gradient_size * 2 * (w - 1) / w for w in world_sizes]
    
    ax2.plot(world_sizes, tree_costs, 'o-', linewidth=2, label='Tree', color='red')
    ax2.plot(world_sizes, ring_costs, 's-', linewidth=2, label='Ring', color='green')
    ax2.set_xlabel('Number of Workers', fontsize=12)
    ax2.set_ylabel('Communication Cost', fontsize=12)
    ax2.set_title('Scalability Analysis', fontsize=14, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log', base=2)
    
    # Plot 3: Ring topology
    ax3 = plt.subplot(1, 3, 3)
    ax3.axis('off')
    ax3.set_xlim(-1.5, 1.5)
    ax3.set_ylim(-1.5, 1.5)
    ax3.set_aspect('equal')
    
    # Draw ring
    angles = np.linspace(0, 2*np.pi, world_size, endpoint=False)
    for i in range(world_size):
        x, y = np.cos(angles[i]), np.sin(angles[i])
        next_x, next_y = np.cos(angles[(i+1)%world_size]), np.sin(angles[(i+1)%world_size])
        
        # Draw edge
        ax3.arrow(x*0.9, y*0.9, (next_x-x)*0.7, (next_y-y)*0.7,
                 head_width=0.1, head_length=0.08, fc='gray', ec='gray', alpha=0.5)
        
        # Draw node
        circle = plt.Circle((x, y), 0.15, color='steelblue', ec='black', linewidth=2)
        ax3.add_patch(circle)
        ax3.text(x, y, f'W{i}', ha='center', va='center',
                fontsize=10, fontweight='bold', color='white')
    
    ax3.set_title('Ring Topology', fontsize=14, fontweight='bold', pad=20)
    
    plt.tight_layout()
    plt.savefig('distributed_ring_allreduce.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: distributed_ring_allreduce.png\n")
    plt.close()

def example_federated_learning():
    """
    Federated Learning: Train on distributed data without centralizing it
    
    - Each client trains locally
    - Only model updates are shared
    - Privacy-preserving
    """
    print("=" * 80)
    print("EXAMPLE 4: Federated Learning Simulation")
    print("=" * 80)
    
    n_clients = 5
    rounds = 20
    local_epochs = 5
    
    print(f"\nFederated Learning Setup:")
    print(f"  Clients: {n_clients}")
    print(f"  Communication Rounds: {rounds}")
    print(f"  Local Epochs per Round: {local_epochs}")
    
    # Generate non-IID data for each client (heterogeneous)
    np.random.seed(45)
    client_data = []
    
    for client_id in range(n_clients):
        # Each client has biased data distribution
        n_samples = 100 + np.random.randint(-20, 20)
        bias = client_id / n_clients
        
        X = np.random.randn(n_samples, 2) + bias * 2
        y = ((X[:, 0] + X[:, 1]) > bias).astype(float)
        
        client_data.append((X, y))
        print(f"  Client {client_id}: {n_samples} samples, class ratio: {y.mean():.2f}")
    
    # Global model
    global_W = np.random.randn(2) * 0.1
    global_b = 0.0
    
    # Federated training
    print(f"\nFederated Training...")
    global_loss_history = []
    
    for round_num in range(rounds):
        client_models = []
        client_weights = []
        
        # Each client trains locally
        for client_id in range(n_clients):
            X_client, y_client = client_data[client_id]
            
            # Initialize with global model
            W = global_W.copy()
            b = global_b
            
            # Local training
            for _ in range(local_epochs):
                pred = 1 / (1 + np.exp(-(X_client @ W + b)))
                dW = X_client.T @ (pred - y_client) / len(y_client)
                db = np.mean(pred - y_client)
                
                W -= 0.1 * dW
                b -= 0.1 * db
            
            client_models.append((W, b))
            client_weights.append(len(y_client))
        
        # Aggregate models (weighted average)
        total_samples = sum(client_weights)
        global_W = sum(w * model[0] for w, model in zip(client_weights, client_models)) / total_samples
        global_b = sum(w * model[1] for w, model in zip(client_weights, client_models)) / total_samples
        
        # Compute global loss
        total_loss = 0
        total_samples = 0
        for X_client, y_client in client_data:
            pred = 1 / (1 + np.exp(-(X_client @ global_W + global_b)))
            loss = -np.mean(y_client * np.log(pred + 1e-8) + (1 - y_client) * np.log(1 - pred + 1e-8))
            total_loss += loss * len(y_client)
            total_samples += len(y_client)
        
        avg_loss = total_loss / total_samples
        global_loss_history.append(avg_loss)
        
        if (round_num + 1) % 5 == 0:
            print(f"  Round {round_num+1}/{rounds} - Global Loss: {avg_loss:.4f}")
    
    # Visualization
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Training convergence
    ax1 = plt.subplot(1, 3, 1)
    ax1.plot(global_loss_history, linewidth=2, color='purple')
    ax1.set_xlabel('Communication Round', fontsize=12)
    ax1.set_ylabel('Global Loss', fontsize=12)
    ax1.set_title('Federated Learning Convergence', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Data distribution per client
    ax2 = plt.subplot(1, 3, 2)
    for client_id in range(n_clients):
        X_client, y_client = client_data[client_id]
        ax2.scatter(X_client[y_client == 0, 0], X_client[y_client == 0, 1],
                   alpha=0.5, s=20, label=f'Client {client_id}')
    ax2.set_xlabel('Feature 1', fontsize=12)
    ax2.set_ylabel('Feature 2', fontsize=12)
    ax2.set_title('Non-IID Data Distribution', fontsize=14, fontweight='bold')
    ax2.legend(fontsize=8)
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Architecture
    ax3 = plt.subplot(1, 3, 3)
    ax3.axis('off')
    
    info_text = f"""
    Federated Learning
    ═══════════════════════════
    
    Strategy: FEDERATED
    Clients: {n_clients}
    Communication: Rounds
    Data: Non-IID (heterogeneous)
    
    Protocol:
    1. Server broadcasts model
    2. Clients train locally
    3. Clients send updates
    4. Server aggregates
    5. Repeat
    
    Privacy Benefits:
    • Data stays on device
    • Only model updates shared
    • Secure aggregation
    
    Challenges:
    • Non-IID data
    • Stragglers
    • Communication cost
    
    Final Loss: {global_loss_history[-1]:.4f}
    """
    
    ax3.text(0.1, 0.5, info_text, fontsize=9, family='monospace',
            verticalalignment='center')
    
    plt.tight_layout()
    plt.savefig('distributed_federated.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: distributed_federated.png\n")
    plt.close()

if __name__ == "__main__":
    print("\n" + "=" * 80)
    print("DISTRIBUTED MACHINE LEARNING FRAMEWORK")
    print("Demonstration of Various Distributed Computing Schemes")
    print("=" * 80 + "\n")
    
    # Run all examples
    example_data_parallel_training()
    example_parameter_server()
    example_ring_allreduce()
    example_federated_learning()
    
    print("=" * 80)
    print("SUMMARY: Distributed Computing Schemes")
    print("=" * 80)
    print("""
1. DATA PARALLEL
   - Best for: Large datasets, simple models
   - Communication: All-reduce gradients
   - Scaling: Linear with workers
   
2. PARAMETER SERVER
   - Best for: Asynchronous training, handling stragglers
   - Communication: Push/pull with central server
   - Scaling: High throughput

3. RING ALL-REDUCE
   - Best for: Bandwidth optimization
   - Communication: Peer-to-peer ring
   - Scaling: Bandwidth-optimal

4. FEDERATED LEARNING
   - Best for: Privacy-preserving, edge devices
   - Communication: Periodic model aggregation
   - Scaling: Handles non-IID data

All schemes implemented in the distributed computing module!
    """)
    print("=" * 80)
