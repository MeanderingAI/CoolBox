use std::ffi::c_void;
use std::marker::PhantomData;
use std::ptr::NonNull;

unsafe extern "C" {
    fn coolbox_bandit_arm_new(probability: f64) -> *mut c_void;
    fn coolbox_bandit_arm_free(handle: *mut c_void);
    fn coolbox_bandit_arm_pull(handle: *mut c_void, reward: *mut f64) -> i32;
    fn coolbox_bandit_arm_update(handle: *mut c_void, reward: f64) -> i32;
    fn coolbox_bandit_arm_estimated_probability(handle: *const c_void) -> f64;
    fn coolbox_bandit_arm_pull_count(handle: *const c_void) -> i32;
    fn coolbox_bandit_arm_true_probability(handle: *const c_void) -> f64;
}

struct BanditHandle {
    ptr: NonNull<c_void>,
    _not_sync: PhantomData<*mut ()>,
}
impl Drop for BanditHandle {
    fn drop(&mut self) {
        unsafe { coolbox_bandit_arm_free(self.ptr.as_ptr()) };
    }
}

pub struct BanditArm {
    handle: BanditHandle,
}
impl BanditArm {
    pub fn new(true_probability: f64) -> Result<Self, String> {
        if !true_probability.is_finite() || !(0.0..=1.0).contains(&true_probability) {
            return Err("bandit reward probability must be finite and between 0 and 1".into());
        }
        let ptr = unsafe { coolbox_bandit_arm_new(true_probability) };
        Ok(Self {
            handle: BanditHandle {
                ptr: NonNull::new(ptr).ok_or("failed to create bandit arm")?,
                _not_sync: PhantomData,
            },
        })
    }
    pub fn pull(&mut self) -> Result<f64, String> {
        let mut reward = 0.0;
        if unsafe { coolbox_bandit_arm_pull(self.handle.ptr.as_ptr(), &mut reward) } == 0 {
            return Err("failed to sample bandit reward".into());
        }
        Ok(reward)
    }
    pub fn update(&mut self, reward: f64) -> Result<(), String> {
        if !reward.is_finite() || !(0.0..=1.0).contains(&reward) {
            return Err("bandit reward must be finite and between 0 and 1".into());
        }
        if unsafe { coolbox_bandit_arm_update(self.handle.ptr.as_ptr(), reward) } == 0 {
            return Err("failed to update bandit arm".into());
        }
        Ok(())
    }
    pub fn estimated_probability(&self) -> f64 {
        unsafe { coolbox_bandit_arm_estimated_probability(self.handle.ptr.as_ptr()) }
    }
    pub fn pull_count(&self) -> usize {
        unsafe { coolbox_bandit_arm_pull_count(self.handle.ptr.as_ptr()).max(0) as usize }
    }
    pub fn true_probability(&self) -> f64 {
        unsafe { coolbox_bandit_arm_true_probability(self.handle.ptr.as_ptr()) }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DistanceMetric {
    Euclidean,
    Manhattan,
    Cosine,
}

pub struct Knn {
    k: usize,
    metric: DistanceMetric,
    training: Vec<Vec<f64>>,
}

impl Knn {
    pub fn new(k: usize, metric: DistanceMetric) -> Result<Self, String> {
        if k == 0 {
            return Err("k must be positive".into());
        }
        Ok(Self {
            k,
            metric,
            training: Vec::new(),
        })
    }
    pub fn fit(&mut self, features: &[Vec<f64>]) -> Result<(), String> {
        let (_, columns) = validate_matrix(features)?;
        if features.len() <= self.k {
            return Err("k must be less than the number of training samples".into());
        }
        if columns == 0 {
            return Err("feature matrix must have at least one column".into());
        }
        self.training = features.to_vec();
        Ok(())
    }
    pub fn k(&self) -> usize {
        self.k
    }
    pub fn metric(&self) -> DistanceMetric {
        self.metric
    }
    pub fn is_fitted(&self) -> bool {
        !self.training.is_empty()
    }

    pub fn k_neighbors(
        &self,
        queries: &[Vec<f64>],
    ) -> Result<(Vec<Vec<usize>>, Vec<Vec<f64>>), String> {
        if !self.is_fitted() {
            return Err("KNN must be fitted before querying neighbors".into());
        }
        if queries.is_empty() {
            return Ok((Vec::new(), Vec::new()));
        }
        let (_, columns) = validate_matrix(queries)?;
        if columns != self.training[0].len() {
            return Err("query feature count does not match training data".into());
        }
        self.neighbors(queries, None)
    }
    pub fn k_neighbors_training(&self) -> Result<(Vec<Vec<usize>>, Vec<Vec<f64>>), String> {
        if !self.is_fitted() {
            return Err("KNN must be fitted before querying neighbors".into());
        }
        let excluded: Vec<usize> = (0..self.training.len()).collect();
        self.neighbors(&self.training, Some(&excluded))
    }
    pub fn pairwise_distances(
        &self,
        x: &[Vec<f64>],
        y: &[Vec<f64>],
    ) -> Result<Vec<Vec<f64>>, String> {
        if x.is_empty() || y.is_empty() {
            return Err("distance matrices must be non-empty".into());
        }
        let (_, x_columns) = validate_matrix(x)?;
        let (_, y_columns) = validate_matrix(y)?;
        if x_columns != y_columns {
            return Err("both matrices must have the same feature count".into());
        }
        Ok(x.iter()
            .map(|left| {
                y.iter()
                    .map(|right| distance(left, right, self.metric))
                    .collect()
            })
            .collect())
    }
    fn neighbors(
        &self,
        queries: &[Vec<f64>],
        excluded: Option<&[usize]>,
    ) -> Result<(Vec<Vec<usize>>, Vec<Vec<f64>>), String> {
        if self.k
            > self
                .training
                .len()
                .saturating_sub(usize::from(excluded.is_some()))
        {
            return Err("fewer than k training samples are available".into());
        }
        let mut indices = Vec::with_capacity(queries.len());
        let mut distances = Vec::with_capacity(queries.len());
        for (query_index, query) in queries.iter().enumerate() {
            let mut candidates: Vec<_> = self
                .training
                .iter()
                .enumerate()
                .filter(|(index, _)| excluded.map_or(true, |values| values[query_index] != *index))
                .map(|(index, row)| (index, distance(query, row, self.metric)))
                .collect();
            candidates.sort_by(|left, right| left.1.total_cmp(&right.1).then(left.0.cmp(&right.0)));
            indices.push(candidates.iter().take(self.k).map(|item| item.0).collect());
            distances.push(candidates.iter().take(self.k).map(|item| item.1).collect());
        }
        Ok((indices, distances))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KMeansInit {
    Random,
    KMeansPlusPlus,
}

#[derive(Debug, Clone, PartialEq)]
pub struct KMeansResult {
    pub centroids: Vec<Vec<f64>>,
    pub labels: Vec<usize>,
    pub inertia_history: Vec<f64>,
    pub inertia: f64,
    pub iterations: usize,
}

pub struct KMeans {
    clusters: usize,
    max_iterations: usize,
    init: KMeansInit,
    random_state: i64,
    result: Option<KMeansResult>,
}

impl KMeans {
    pub fn new(
        clusters: usize,
        max_iterations: usize,
        init: KMeansInit,
        random_state: i64,
    ) -> Result<Self, String> {
        if clusters == 0 {
            return Err("number of clusters must be positive".into());
        }
        Ok(Self {
            clusters,
            max_iterations,
            init,
            random_state,
            result: None,
        })
    }
    pub fn default_model() -> Self {
        Self {
            clusters: 3,
            max_iterations: 300,
            init: KMeansInit::KMeansPlusPlus,
            random_state: 42,
            result: None,
        }
    }
    pub fn fit(&mut self, features: &[Vec<f64>]) -> Result<(), String> {
        validate_matrix(features)?;
        if features.len() < self.clusters {
            return Err("number of samples must be at least the number of clusters".into());
        }
        let mut rng = SplitMix64(self.random_state as u64);
        let mut centroids = self.initialize(features, &mut rng);
        let mut history = Vec::with_capacity(self.max_iterations);
        let mut iterations = 0;
        while iterations < self.max_iterations {
            let labels = assign(features, &centroids);
            history.push(inertia(features, &centroids, &labels));
            let next = update_centroids(features, &labels, self.clusters, features[0].len());
            iterations += 1;
            let converged = next
                .iter()
                .zip(&centroids)
                .all(|(left, right)| left.iter().zip(right).all(|(a, b)| (a - b).abs() <= 1e-9));
            centroids = next;
            if converged {
                break;
            }
        }
        let labels = assign(features, &centroids);
        self.result = Some(KMeansResult {
            inertia: inertia(features, &centroids, &labels),
            centroids,
            labels,
            inertia_history: history,
            iterations,
        });
        Ok(())
    }
    pub fn fit_predict(&mut self, features: &[Vec<f64>]) -> Result<Vec<usize>, String> {
        self.fit(features)?;
        Ok(self.result()?.labels.clone())
    }
    pub fn predict(&self, features: &[Vec<f64>]) -> Result<Vec<usize>, String> {
        let result = self
            .result
            .as_ref()
            .ok_or("KMeans model must be fitted before prediction")?;
        let (_, columns) = validate_matrix(features)?;
        if columns != result.centroids[0].len() {
            return Err("feature count does not match fitted KMeans model".into());
        }
        Ok(assign(features, &result.centroids))
    }
    pub fn result(&self) -> Result<&KMeansResult, String> {
        self.result
            .as_ref()
            .ok_or_else(|| "KMeans model must be fitted before accessing results".into())
    }
    pub fn centroids(&self) -> Result<&[Vec<f64>], String> {
        Ok(&self.result()?.centroids)
    }
    pub fn labels(&self) -> Result<&[usize], String> {
        Ok(&self.result()?.labels)
    }
    pub fn inertia(&self) -> Result<f64, String> {
        Ok(self.result()?.inertia)
    }
    pub fn inertia_history(&self) -> Result<&[f64], String> {
        Ok(&self.result()?.inertia_history)
    }
    pub fn iterations(&self) -> Result<usize, String> {
        Ok(self.result()?.iterations)
    }
    pub fn is_fitted(&self) -> bool {
        self.result.is_some()
    }
    fn initialize(&self, features: &[Vec<f64>], rng: &mut SplitMix64) -> Vec<Vec<f64>> {
        if self.init == KMeansInit::Random {
            let mut indices: Vec<_> = (0..features.len()).collect();
            for i in (1..indices.len()).rev() {
                let j = rng.range(i + 1);
                indices.swap(i, j);
            }
            return indices[..self.clusters]
                .iter()
                .map(|&i| features[i].clone())
                .collect();
        }
        let mut centroids = vec![features[rng.range(features.len())].clone()];
        let mut minimum_distances = vec![0.0_f64; features.len()];
        for _ in 1..self.clusters {
            let mut total = 0.0;
            for (index, row) in features.iter().enumerate() {
                let distance = squared_distance(row, centroids.last().unwrap());
                minimum_distances[index] = if centroids.len() > 1 {
                    minimum_distances[index].min(distance)
                } else {
                    distance
                };
                total += minimum_distances[index];
            }
            let mut chosen = 0;
            if total.is_finite() && total > 0.0 {
                let mut threshold = rng.unit() * total;
                for (index, value) in minimum_distances.iter().enumerate() {
                    threshold -= value;
                    if threshold <= 0.0 {
                        chosen = index;
                        break;
                    }
                }
            }
            centroids.push(features[chosen].clone());
        }
        centroids
    }
}

fn validate_matrix(features: &[Vec<f64>]) -> Result<(usize, usize), String> {
    let columns = features.first().map(Vec::len).unwrap_or(0);
    if features.is_empty() || columns == 0 || features.iter().any(|row| row.len() != columns) {
        return Err("feature matrix must be non-empty and rectangular".into());
    }
    if features.iter().flatten().any(|value| !value.is_finite()) {
        return Err("feature matrix values must be finite".into());
    }
    Ok((features.len(), columns))
}
fn squared_distance(left: &[f64], right: &[f64]) -> f64 {
    left.iter().zip(right).map(|(a, b)| (a - b) * (a - b)).sum()
}
fn distance(left: &[f64], right: &[f64], metric: DistanceMetric) -> f64 {
    match metric {
        DistanceMetric::Euclidean => squared_distance(left, right).sqrt(),
        DistanceMetric::Manhattan => left.iter().zip(right).map(|(a, b)| (a - b).abs()).sum(),
        DistanceMetric::Cosine => {
            let (mut dot, mut left_norm, mut right_norm) = (0.0, 0.0, 0.0);
            for (a, b) in left.iter().zip(right) {
                dot += a * b;
                left_norm += a * a;
                right_norm += b * b;
            }
            if left_norm == 0.0 || right_norm == 0.0 {
                return 1.0;
            }
            1.0 - (dot / (left_norm.sqrt() * right_norm.sqrt())).clamp(-1.0, 1.0)
        }
    }
}
fn assign(features: &[Vec<f64>], centroids: &[Vec<f64>]) -> Vec<usize> {
    features
        .iter()
        .map(|row| {
            centroids
                .iter()
                .enumerate()
                .min_by(|(_, a), (_, b)| {
                    squared_distance(row, a).total_cmp(&squared_distance(row, b))
                })
                .map(|(index, _)| index)
                .unwrap_or(0)
        })
        .collect()
}
fn update_centroids(
    features: &[Vec<f64>],
    labels: &[usize],
    count: usize,
    columns: usize,
) -> Vec<Vec<f64>> {
    let mut centroids = vec![vec![0.0; columns]; count];
    let mut counts = vec![0; count];
    for (row, &cluster) in features.iter().zip(labels) {
        counts[cluster] += 1;
        for (sum, value) in centroids[cluster].iter_mut().zip(row) {
            *sum += value;
        }
    }
    for (centroid, &count) in centroids.iter_mut().zip(&counts) {
        if count != 0 {
            for value in centroid {
                *value /= count as f64;
            }
        }
    }
    centroids
}
fn inertia(features: &[Vec<f64>], centroids: &[Vec<f64>], labels: &[usize]) -> f64 {
    features
        .iter()
        .zip(labels)
        .map(|(row, &cluster)| squared_distance(row, &centroids[cluster]))
        .sum()
}
struct SplitMix64(u64);
impl SplitMix64 {
    fn next(&mut self) -> u64 {
        self.0 = self.0.wrapping_add(0x9e3779b97f4a7c15);
        let mut value = self.0;
        value = (value ^ (value >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
        value = (value ^ (value >> 27)).wrapping_mul(0x94d049bb133111eb);
        value ^ (value >> 31)
    }
    fn range(&mut self, upper: usize) -> usize {
        (self.next() as u128 * upper as u128 >> 64) as usize
    }
    fn unit(&mut self) -> f64 {
        (self.next() >> 11) as f64 / (1u64 << 53) as f64
    }
}
