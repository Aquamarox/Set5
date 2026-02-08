import os
import math
import pandas as pd
import matplotlib.pyplot as plt


def ensure_dir(d):
    os.makedirs(d, exist_ok=True)


def main():
    print("Построение графиков HyperLogLog")
    run_csv = input("Введите путь к run CSV (например run_p12.csv): ").strip()
    summary_csv = input("Введите путь к summary CSV (например summary_p12.csv): ").strip()
    outdir = input("Введите папку для сохранения результатов (например report/p12): ").strip()

    stream_id = int(input("Введите stream_id для графика №1 (например 0): ").strip())
    p = int(input("Введите значение p (например 12): ").strip())

    ensure_dir(outdir)

    # Загрузка данных
    run_df = pd.read_csv(run_csv)
    summary_df = pd.read_csv(summary_csv)

    # === График 1: сравнение точного значения и оценки для одного потока ===
    stream_data = run_df[run_df["stream_id"] == stream_id].copy()
    stream_data = stream_data.sort_values("checkpoint")

    plt.figure(figsize=(10, 6))
    plt.plot(stream_data["processed"], stream_data["exact"],
             'g-', linewidth=2.5, label='Точное значение $F_t^0$')
    plt.plot(stream_data["processed"], stream_data["estimate"],
             'b--', linewidth=2, label='Оценка $\\hat{N}_t$ (HyperLogLog)')
    plt.xlabel('Обработано элементов', fontsize=12)
    plt.ylabel('Количество уникальных элементов', fontsize=12)
    plt.title(f'Поток #{stream_id}: точное значение vs оценка HyperLogLog', fontsize=14)
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    fig1_path = os.path.join(outdir, "fig1_stream.png")
    plt.savefig(fig1_path, dpi=150)
    plt.close()
    print(f"✓ Сохранён график 1: {fig1_path}")

    # === График 2: статистика по всем потокам (среднее ± σ) ===
    summary_df = summary_df.sort_values("checkpoint")

    # Вычисляем относительную ошибку для анализа
    summary_df["rel_error"] = (summary_df["estimate_mean"] - summary_df["exact_mean"]).abs() / summary_df["exact_mean"]

    plt.figure(figsize=(10, 6))
    plt.plot(summary_df["processed"], summary_df["exact_mean"],
             'g-', linewidth=2.5, label='Точное значение (среднее)')
    plt.plot(summary_df["processed"], summary_df["estimate_mean"],
             'b-', linewidth=2, label='$E[\\hat{N}_t]$')
    plt.fill_between(summary_df["processed"],
                     summary_df["estimate_mean"] - summary_df["estimate_std"],
                     summary_df["estimate_mean"] + summary_df["estimate_std"],
                     alpha=0.3, color='blue', label='$\\pm \\sigma$')
    plt.xlabel('Обработано элементов', fontsize=12)
    plt.ylabel('Количество уникальных элементов', fontsize=12)
    plt.title('HyperLogLog: статистика по 100 потокам', fontsize=14)
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    fig2_path = os.path.join(outdir, "fig2_mean_std.png")
    plt.savefig(fig2_path, dpi=150)
    plt.close()
    print(f"✓ Сохранён график 2: {fig2_path}")

    # === Анализ точности ===
    m = 2 ** p
    theoretical_std_104 = 1.04 / math.sqrt(m)
    theoretical_std_132 = 1.32 / math.sqrt(m)

    # Эмпирическая относительная дисперсия (усреднённая по шагам)
    rel_std = summary_df["estimate_std"] / summary_df["estimate_mean"]
    rel_std_mean = rel_std.mean()
    rel_std_max = rel_std.max()

    # Средняя абсолютная относительная ошибка
    mean_rel_error = summary_df["rel_error"].mean()

    # Сохранение анализа
    analysis_path = os.path.join(outdir, "analysis.txt")
    with open(analysis_path, "w", encoding="utf-8") as f:
        f.write("=" * 60 + "\n")
        f.write("АНАЛИЗ ТОЧНОСТИ HYPERLOGLOG\n")
        f.write("=" * 60 + "\n\n")

        f.write("Параметры эксперимента:\n")
        f.write(f"  p (бит индекса регистра)   = {p}\n")
        f.write(f"  m (количество регистров)   = {m}\n")
        f.write(f"  Потоков                    = {run_df['stream_id'].max() + 1}\n")
        f.write(f"  Длина потока               = {int(run_df['processed'].max())}\n")
        f.write(f"  Размер вселенной (уникальных) = {int(summary_df['exact_mean'].iloc[-1])}\n\n")

        f.write("Теоретические границы относительной ошибки:\n")
        f.write(f"  Стандартная оценка: 1.04/√m = {theoretical_std_104:.4f} ({theoretical_std_104 * 100:.2f}%)\n")
        f.write(f"  Консервативная оценка: 1.32/√m = {theoretical_std_132:.4f} ({theoretical_std_132 * 100:.2f}%)\n\n")

        f.write("Эмпирические результаты (усреднённые по шагам):\n")
        f.write(f"  Средняя относительная дисперсия σ/E[N̂] = {rel_std_mean:.4f} ({rel_std_mean * 100:.2f}%)\n")
        f.write(f"  Максимальная относительная дисперсия   = {rel_std_max:.4f} ({rel_std_max * 100:.2f}%)\n")
        f.write(f"  Средняя абсолютная ошибка |E[N̂]-F⁰|/F⁰ = {mean_rel_error:.4f} ({mean_rel_error * 100:.2f}%)\n\n")

        f.write("Выводы:\n")
        if rel_std_mean <= theoretical_std_132 * 1.1:
            f.write(f"  ✓ Эмпирическая дисперсия ({rel_std_mean * 100:.2f}%) укладывается в теоретическую границу\n")
            f.write(f"    1.32/√m = {theoretical_std_132 * 100:.2f}%\n")
        else:
            f.write(f"  ✗ Эмпирическая дисперсия ({rel_std_mean * 100:.2f}%) превышает теоретическую границу\n")
            f.write(f"    1.32/√m = {theoretical_std_132 * 100:.2f}%\n")

        f.write("\nПамять:\n")
        if 'packed' in run_csv.lower() or 'packed' in summary_csv.lower():
            packed_bytes = (m * 6 + 7) // 8
            f.write(f"  Упакованная версия: {packed_bytes} байт ({packed_bytes / 1024:.2f} КБ)\n")
            f.write(f"  Стандартная версия: {m} байт ({m / 1024:.2f} КБ)\n")
            f.write(f"  Экономия памяти: {(1 - packed_bytes / m) * 100:.1f}%\n")
        else:
            f.write(f"  Стандартная версия: {m} байт ({m / 1024:.2f} КБ)\n")

    print(f"✓ Сохранён анализ: {analysis_path}")
    print("\n" + "=" * 60)
    print("Готово! Результаты сохранены в папке:", outdir)
    print("=" * 60)


if __name__ == "__main__":
    main()