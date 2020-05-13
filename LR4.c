#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>

#define PARENT "\033[0;31mОсновной:\033[0m"
#define CHILD1 "\033[0;32mПервый процесс:\033[0m"
#define CHILD2 "\033[0;33mВторой процесс:\033[0m"
#define STRING_SIZE 40
#define VARIANT 17

int main()
{
    char *init;
    pid_t child1, child2, child3;
    sigset_t sigset;
    siginfo_t receive_val;
    char user_string[STRING_SIZE];
    int str_size;
    int temp;
    child1 = child2 = child3 = 0;

    // Создаём процессы
    child1 = fork();
    if (child1 != 0)
    {
        child2 = fork();
    }
    if (child2 != 0)
    {
        child3 = fork();
    }

    if (child1 * child2 * child3 != 0) // Основной процесс
    {
        init = PARENT;

        // Ввод строки
        printf("%s введите строку:\n", init);
        scanf("%s", user_string);

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sigset, 0);

        // Отправляем строку в 1 процесс
        printf("%s отправляем строку в первый процесс %d.\n", init, child1);
        union sigval send_str;
        for (int i = 0; i < STRING_SIZE; i++)
        {
            send_str.sival_int = user_string[i];
            sigqueue(child1, SIGUSR1, send_str);
            sigwait(&sigset, &temp);
        }
        printf("%s строка отправлена.\n", init);

        // Принимаем размер строки из 1 процесса
        sigwaitinfo(&sigset, &receive_val);
        str_size = receive_val.si_value.sival_int;
        sleep(1);
        printf("%s длина: %d, отправляем во второй процесс.\n", init, str_size);

        // Отправляем длину строки во 2 процесс
        setpgid(child2, 0);
        union sigval trans_size;
        trans_size.sival_int = str_size;
        if (!sigqueue(child2, SIGUSR1, trans_size))
        {
            printf("%s размер отправлен.\n", init);
        }

        sleep(10);
        printf("%s я проснулся.\n", init);

        pid_t group_pid = getpgid(child2);
        killpg(group_pid, SIGKILL);

        waitpid(child1, 0, 0);
        waitpid(child2, 0, 0);

        kill(child3, SIGINT);
        system("ps -x");
        kill(child3, SIGKILL);
        waitpid(child3, 0, 0);
    }
    else
    {
        printf("Я дочерний процесс. Мой pid %d\n", getpid());
        if (child1 + child2 + child3 == 0) // Первый процесс
        {
            init = CHILD1;

            sigemptyset(&sigset);
            sigaddset(&sigset, SIGUSR1);
            sigprocmask(SIG_BLOCK, &sigset, 0);

            for (int i = 0; i < STRING_SIZE; i++)
            {
                sigwaitinfo(&sigset, &receive_val);
                user_string[i] = (char)receive_val.si_value.sival_int;
                kill(getppid(), SIGUSR1);
            }
            str_size = strnlen(user_string, STRING_SIZE);
            sleep(1);
            printf("%s переданная строка: %s, длина строки %d, отправляем основному процессу.\n", init, user_string, str_size);

            union sigval trans_size;
            trans_size.sival_int = str_size;
            if (!sigqueue(getppid(), SIGUSR1, trans_size))
            {
                printf("%s размер отправлен.\n", init);
            }
        }
        else if (child2 + child3 == 0) // Второй процесс
        {
            init = CHILD2;

            sigemptyset(&sigset);
            sigaddset(&sigset, SIGUSR1);
            sigprocmask(SIG_BLOCK, &sigset, 0);

            // Получаем длину строки из основного процесса
            sigwaitinfo(&sigset, &receive_val);
            str_size = receive_val.si_value.sival_int;
            sleep(1);
            printf("%s длина: %d.\n", init, str_size);

            int new_str_size = str_size - VARIANT;
            if (new_str_size > 0)
            {
                printf("%s будет создано %d дочерних процессов, моя группа %d.\n", init, new_str_size, getpgid(0));
                pid_t new_child = 0;

                for (int i = 0; i < new_str_size; i++)
                {
                    if (new_child == 0)
                    {
                        switch (new_child = fork())
                        {
                        case 0:
                            new_child = -1;
                            printf("%s дочерний процесс №%d, мой pid: %d, моя группа: %d.\n", init, i, getpid(), getpgid(0));
                            temp = 0;
                            while (1)
                            {
                                sleep(1);
                                printf("%s №%d: процесс жив %d.\n", init, i, temp++);
                            }
                            break;
                        case -1:
                            printf("%s процесс не был создан.\n", init);
                            i--;
                            new_child = 0;
                            break;
                        default:
                            printf("%s был создан дочерний процесс №%d, его pid: %d, его группа: %d.\n", init, i, getpid(), getpgid(0));
                            new_child = 0;
                            break;
                        }
                    }
                }
            }
            else
            {
                printf("%s невозможно создать дочерние процессы!\n", init);
            }
        }
        else // Третий процесс
        {
            sigemptyset(&sigset);
            sigaddset(&sigset, SIGINT);
            signal(SIGINT, SIG_IGN);
            sigsuspend(&sigset);
        }
    }
    return 0;
}
