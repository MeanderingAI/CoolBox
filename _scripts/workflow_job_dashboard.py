import yaml
from pathlib import Path
from collections import defaultdict
import networkx as nx
import matplotlib.pyplot as plt

# Path to your workflow file
yaml_path = Path('.github/workflows/build-purchase-pipeline.yaml')

def load_workflow_jobs(yaml_path):
    with open(yaml_path, 'r') as f:
        data = yaml.safe_load(f)
    jobs = data.get('jobs', {})
    return jobs

def build_job_graph(jobs):
    G = nx.DiGraph()
    for job, details in jobs.items():
        G.add_node(job)
        needs = details.get('needs', [])
        if isinstance(needs, str):
            needs = [needs]
        for dep in needs:
            G.add_edge(dep, job)
    return G

def plot_job_graph(G):
    plt.figure(figsize=(12, 8))
    pos = nx.spring_layout(G, seed=42)
    nx.draw(G, pos, with_labels=True, node_color='skyblue', node_size=2000, font_size=10, arrowsize=20)
    plt.title('GitHub Actions Workflow Job Dependency Graph')
    plt.show()

def main():
    jobs = load_workflow_jobs(yaml_path)
    G = build_job_graph(jobs)
    plot_job_graph(G)

if __name__ == '__main__':
    main()
