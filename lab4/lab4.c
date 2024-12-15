#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/math64.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");

#define PROC_FILENAME "tsulab"

int calculate_hypotenuse(int a, int b);
static struct proc_dir_entry *proc_file;

// Функция вычисления гипотенузы
int calculate_hypotenuse(int a, int b) {
    return int_sqrt(a * a + b * b);
}

// Функция чтения для /proc/tsulab
static ssize_t tsulab_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct timespec64 ts;
    struct tm tm;
    char result[128];
    int len;

    // Получаем текущее время
    ktime_get_real_ts64(&ts);
    
    // Преобразуем в локальное время, учитывая смещение часового пояса
    extern struct timezone sys_tz;
    sys_tz.tz_minuteswest = 420;            // Смещение на +7 часов от UTC для томского времени

    // Преобразуем в локальное время с учетом смещения
    time64_to_tm(ts.tv_sec + sys_tz.tz_minuteswest * 60, 0, &tm);

    // Преобразуем время в часы и минуты
    unsigned long hours = tm.tm_hour;
    unsigned long minutes = tm.tm_min;

    // Вычисляем гипотенузу
    unsigned long hypotenuse = calculate_hypotenuse(hours, minutes);

    // Форматируем результат для вывода
    len = snprintf(result, sizeof(result), "\nTime: %02lu:%02lu, Hypotenuse: %lu\n\n",
                   hours, minutes, hypotenuse);

    if (*ppos > 0 || count < len)
        return 0;

    if (copy_to_user(buf, result, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

// Операции для /proc/tsulab
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops tsulab_fops = {
    .proc_read = tsulab_read,
};
#else
static const struct proc_ops tsulab_fops = {
    .read = tsulab_read,
};
#endif

static int __init tsu_init(void) {
    pr_info("Welcome to the Tomsk State University\n");

    // Создаем /proc/tsulab
    proc_file = proc_create(PROC_FILENAME, 0444, NULL, &tsulab_fops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROC_FILENAME);
    return 0;
}

static void __exit tsu_exit(void) {
    // Удаляем /proc/tsulab
    if (proc_file) {
        proc_remove(proc_file);
        pr_info("/proc/%s removed\n", PROC_FILENAME);
    }

    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_init);
module_exit(tsu_exit);
