import numpy as np
import scipy.linalg as SLA
import galois
import math

N = 128
K = 64
n = int(math.log2(N))
pt_file_path = f"./conf/cde/awgn_polar_codes/PT/{n}/N{N}_K{K}_PT.txt"
rp_file_path = f"./conf/cde/awgn_polar_codes/PT/{n}/N{N}_K{K}_RP.txt"
poly_reg = [1, 0, 1, 1, 0, 1, 1]
poly_reg = poly_reg + [0] * (N - len(poly_reg))


def RM_rp():
    A = np.asarray([[1, 0], [1, 1]])
    G = A.copy()
    for i in range(1, n):
        G = np.kron(G, A)

    wghts = np.sum(G, axis=1)
    info_set = np.argsort(wghts)[N - K :]
    info_set = np.sort(info_set)

    return info_set


def write_aff3ct_rateprofile(N, fileName, rate_profile, ebno="*"):
    fb = np.setdiff1d(range(N), rate_profile)
    all_ch = np.concatenate((rate_profile, fb))
    with open(fileName, "w") as o_file:
        o_file.write(f"{N}\n")
        o_file.write(f"awgn\n")
        o_file.write(f"{ebno}\n")
        st = " ".join(str(f) for f in all_ch)
        o_file.write(st)


def compute_pct(T, I):
    import galois

    GF2 = galois.GF(2)
    N = T.shape[0]
    F = np.setdiff1d(range(N), T)
    U = np.eye(N)
    logical_vec = np.zeros(N, dtype="uint8")
    logical_vec[I] = 1
    for i in I:
        U[i, :] = np.logical_and(T[i, :], logical_vec)

    # S = np.linalg.solve(U, T)
    U_gf = GF2(U.astype("uint8"))
    T_gf = GF2(T.astype("uint8"))
    S_gf = np.linalg.solve(U_gf, T_gf)
    S = np.asarray(S_gf)[np.sort(I), :]
    return S, U, T


def write_pre_transform_matrix(Mdf, rp, fileName):
    y, x = np.where(Mdf)
    dfb = np.setdiff1d(np.unique(x), rp)
    with open(fileName, "w") as o_file:
        for xi in dfb:
            yis = np.where(x == xi)[0]
            cis = y[yis]
            cis_str = " ".join(str(ui) for ui in cis)
            cis_str = f"{xi} {cis_str}"
            o_file.write(cis_str)
            o_file.write("\n")


def main():
    T = np.triu(SLA.toeplitz(poly_reg))
    info_bits = RM_rp()
    Mdf, _, _ = compute_pct(T, info_bits)

    write_aff3ct_rateprofile(N, rp_file_path, info_bits)
    print(f"Done writing the rate profile at: {rp_file_path}")

    write_pre_transform_matrix(Mdf, info_bits, pt_file_path)
    print(f"Done writing pre-transform matrix: {pt_file_path}")


if __name__ == "__main__":
    SystemExit(main())
