import argparse
import json
import re
from pathlib import Path
import matplotlib.pyplot as plt

def load_benchmarks(path:Path):
    with path.open() as f:
        data = json.load(f)
    return data["benchmarks"]

def parse_build_time(benchmarks):
    results = []
    pattern = re.compile(r"test_build_time\[(?P<N>[\d_]+)\]")

    for b in benchmarks:
        name = b["name"]
        m = pattern.search(name)
        if not m:
            continue
        N = int(m.group("N").replace("_", ""))
        mean_s = b["stats"]["mean"]
        results.append((N, mean_s))
    results.sort(key=lambda x: x[0])
    return results

def parse_query_and_baseline(benchmarks):
    pattern_query = re.compile(
        r"test_query_latency_and_qps\[(?P<N>[\d_]+)-(?P<Q>[\d_]+)-(?P<win>[\d\.]+)\]"
    )
    pattern_linear = re.compile(
        r"test_linear_baseline_latency_and_qps\[(?P<N>[\d_]+)-(?P<Q>[\d_]+)-(?P<win>[\d\.]+)\]"
    )

    table = {}
    for b in benchmarks:
        name = b["name"]
        mean_s = b["stats"]["mean"]

        mq = pattern_query.search(name)
        if mq:
            N = int(mq.group("N").replace("_",""))
            win = float(mq.group("win"))
            key = (N, win)
            table.setdefault(key, {})["rtree"] = mean_s
            continue
        
        ml = pattern_linear.search(name)
        if ml:
            N = int(ml.group("N").replace("_", ""))
            win = float(ml.group("win"))
            key = (N, win)
            table.setdefault(key, {})["linear"] = mean_s
            continue
    return table

def parse_mixed_workload(benchmarks):
    pattern = re.compile(
        r"test_mixed_workload_scaling\[(?P<N>[\d_]+)-(?P<steps>[\d_]+)\]"
    )
    results = []

    for b in benchmarks:
        name = b["name"]
        m = pattern.search(name)
        if not m:
            continue
        N = int(m.group("N").replace("_", ""))
        steps = int(m.group("steps").replace("_", ""))
        mean_s = b["stats"]["mean"]
        results.append((N, steps, mean_s))
    results.sort(key=lambda x: x[0])
    return results

def parse_fixed_win_line(benchmarks):
    pattern_query = re.compile(
        r"test_query_fixed_win_1pct\[(?P<N>[\d_]+)\]"
    )
    pattern_linear = re.compile(
        r"test_linear_fixed_win_1pct\[(?P<N>[\d_]+)\]"
    )

    table = {}

    for b in benchmarks:
        name = b["name"]
        mean_s = b["stats"]["mean"]

        mq = pattern_query.search(name)
        if mq:
            N = int(mq.group("N").replace("_", ""))
            table.setdefault(N, {})["rtree"] = mean_s
            continue

        ml = pattern_linear.search(name)
        if ml:
            N = int(ml.group("N").replace("_", ""))
            table.setdefault(N, {})["linear"] = mean_s
            continue
    
    Ns = sorted(table.keys())
    rtree_mean = [table[N].get("rtree", float("nan")) for N in Ns]
    linear_mean = [table[N].get("linear", float("nan")) for N in Ns]
    return (Ns, rtree_mean, linear_mean)

def plot_build_time(build_data, out_dir: Path):
    if not build_data:
        print("No build_time benchmarks found.")
        return
    
    N_vals = [n for n, _ in build_data]
    times = [t for _, t in build_data]

    plt.figure()
    plt.plot(N_vals, times, marker="o")
    plt.xlabel("Number of objects (N)")
    plt.ylabel("Build time (seconds)")
    plt.title("R-tree build time vs. N")
    plt.grid(True)
    out_path = out_dir / "build_time_vs_N.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close()
    print(f"[plot] saved {out_path}")

def plot_query_vs_baseline(q_table, out_dir: Path):
    if not q_table:
        print("No query / baseline benchmarks found.")
        return

    for (N, win), vals in sorted(q_table.items()):
        rtree_ms = vals.get("rtree", None)
        base_ms = vals.get("linear", None)
        if rtree_ms is None or base_ms is None:
            continue

        labels = ["R-tree", "Linear scan"]
        means_ms = [rtree_ms * 1e3, base_ms * 1e3]

        plt.figure()
        x = [0, 1]
        plt.bar(x, means_ms)
        plt.xticks(x, labels)
        plt.ylabel("Per-query latency (ms)")
        plt.title(f"N={N}, window={win*100:.0f}%")

        for xi, v in zip(x, means_ms):
            plt.text(xi, v, f"{v:.2f} ms", ha="center", va="bottom", fontsize=8)

        plt.grid(axis="y", linestyle="--", alpha=0.5)
        out_path = out_dir / f"query_vs_baseline_N{N}_win{int(win*100)}.png"
        plt.savefig(out_path, bbox_inches="tight", dpi=150)
        plt.close()
        print(f"[plot] saved {out_path}")

def plot_mixed_workload(mixed_data, out_dir: Path):
    if not mixed_data:
        print("No mixed workload benchmarks found.")
        return 
    
    N_vals = []
    ops_vals = []

    for N, steps, mean_s in mixed_data:
        N_vals.append(N)
        ops = steps / mean_s if mean_s > 0 else float("inf")
        ops_vals.append(ops)

    plt.figure()
    plt.plot(N_vals, ops_vals, marker="o")
    plt.xlabel("Active objects (N_active)")
    plt.ylabel("Operations per second (OPS)")
    plt.title("Mixed workload throughput vs. active objects")
    plt.grid(True)
    out_path = out_dir / "mixed_workload_scaling.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close()
    print(f"[plot] saved {out_path}")

def plot_fixed_win_line(fixed_win_data, out_dir: Path):
    Ns = fixed_win_data[0]
    rtree_mean_s = fixed_win_data[1]
    linear_mean_s = fixed_win_data[2]
    if not Ns:
        print("No fixed-win (1%) benchmarks found.")
        return
    
    rtree_ms = [t * 1e3 for t in rtree_mean_s]
    linear_ms = [t * 1e3 for t in linear_mean_s]

    plt.figure()
    plt.plot(Ns, rtree_ms, marker='o', linestyle='-', label="R-tree")
    plt.plot(Ns, linear_ms, marker='x', linestyle='--', label="Linear scan")
    plt.xlabel("Number of objects (N)")
    plt.ylabel("Per-query latency (ms)")
    plt.title("Fixed 1% window: R-tree vs. linear scan")
    plt.grid(True, which="both", linestyle='--', alpha=0.5)
    plt.legend()
    out_path = out_dir / "fixed_win_1pct_line.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close()
    print(f"[plot] saved {out_path}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "json_path",
        type=Path,
        help="Path to pytest-benchmark JSON output."
    )
    parser.add_argument(
        "--out-dir", 
        type=Path,
        default=Path("docs/figs"),
        help="Directory to store generated plots"
    )
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)

    benchs = load_benchmarks(args.json_path)

    build_data = parse_build_time(benchs)
    q_table = parse_query_and_baseline(benchs)
    mixed_data = parse_mixed_workload(benchs)
    fixed_data = parse_fixed_win_line(benchs)

    plot_build_time(build_data, args.out_dir)
    plot_query_vs_baseline(q_table, args.out_dir)
    plot_mixed_workload(mixed_data, args.out_dir)
    plot_fixed_win_line(fixed_data, args.out_dir)

if __name__ == "__main__":
    main()