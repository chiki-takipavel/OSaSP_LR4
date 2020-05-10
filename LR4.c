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
#define CHILD3 "\033[0;34mТретий процесс:\033[0m"
#define STRING_SIZE 40
#define VARIANT 17

int main()
{
    char *init;
    pid_t child1, child2, child3;
    child1 = child2 = child3 = 0;
    sigset_t sigset;
    char user_string[STRING_SIZE];
    int str_size;
    char one_char;
    int temp;

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
        printf("%s введите строку:\n", init);
        scanf("%s", user_string);
        printf("%s отправляем в первый процесс %d.\n", init, child1);

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigaddset(&sigset, SIGUSR2);
        sigprocmask(SIG_BLOCK, &sigset, 0);

        // Отправляем строку в 1 процесс
        for (int i = 0; i < STRING_SIZE; i++)
        {
            one_char = user_string[i];
            for (int j = 0; j < sizeof(char) * 8; j++)
            {
                kill(child1, ((one_char >> j) & 0x01) == 0 ? SIGUSR1 : SIGUSR2);
                sigwait(&sigset, &temp);
            }
        }
        printf("%s строка отправлена.\n", init);
        kill(child1, SIGUSR1);

        // Принимаем размер строки из 1 процесса
        str_size = 0;
        for (int j = 0; j < sizeof(int) * 8; j++)
        {
            sigwait(&sigset, &temp);
            temp = ((temp == SIGUSR1) ? 0 : 1);
            str_size |= (temp << j);
            kill(child1, SIGUSR1);
        }
        sigwait(&sigset, &temp);
        printf("%s длина: %d, отправляем во второй процесс.\n", init, str_size);

        // Отправляем длину строки во 2 процесс
        setpgid(child2, 0);
        for (int j = 0; j < sizeof(int) * 8; j++)
        {
            kill(child2, ((str_size >> j) & 0x01) == 0 ? SIGUSR1 : SIGUSR2);
            sigwait(&sigset, &temp);
        }
        printf("%s размер отправлен.\n", init);
        kill(child2, SIGUSR1);

        sleep(10);
        printf("%s я проснулся.\n", init);

        pid_t group_pid = getpgid(child2);
        killpg(group_pid, SIGKILL);

        waitpid(child1, 0, 0);
        waitpid(child2, 0, 0);

        sigfillset(&sigset);
        sigprocmask(SIG_BLOCK, &sigset, 0);
        kill(child3, SIGINT);
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
            sigaddset(&sigset, SIGUSR2);
            sigprocmask(SIG_BLOCK, &sigset, 0);

            for (int i = 0; i < STRING_SIZE; i++)
            {
                one_char = 0;
                for (int j = 0; j < sizeof(char) * 8; j++)
                {
                    if (!sigwait(&sigset, &temp))
                    {
                        temp = ((temp == SIGUSR1) ? 0 : 1);
                        one_char |= (temp << j);
                        kill(getppid(), SIGUSR1);
                    }
                    else
                    {
                        j--;
                    }
                }
                user_string[i] = one_char;
            }
            str_size = strnlen(user_string, STRING_SIZE);
            sigwait(&sigset, &temp);
            printf("%s переданная строка: %s, длина строки %d, отправляем основному процессу.\n", init, user_string, str_size);

            for (int j = 0; j < sizeof(int) * 8; j++)
            {
                kill(getppid(), ((str_size >> j) & 0x01) == 0 ? SIGUSR1 : SIGUSR2);
                sigwait(&sigset, &temp);
            }
            printf("%s отправлено.\n", init);
            kill(getppid(), SIGUSR1);
        }
        else if (child2 + child3 == 0) // Второй процесс
        {
            init = CHILD2;

            sigemptyset(&sigset);
            sigaddset(&sigset, SIGUSR1);
            sigaddset(&sigset, SIGUSR2);
            sigprocmask(SIG_BLOCK, &sigset, 0);

            // Получаем длину строки из основного процесса
            str_size = 0;
            for (int j = 0; j < sizeof(int) * 8; j++)
            {
                sigwait(&sigset, &temp);
                temp = ((temp == SIGUSR1) ? 0 : 1);
                str_size |= (temp << j);
                kill(getppid(), SIGUSR1);
            }
            sigwait(&sigset, &temp);
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
                        new_child = fork();
                        if (new_child == 0)
                        {
                            sleep(1);
                            new_child = -1;
                            printf("%s дочерний процесс №%d, мой pid: %d, моя группа: %d.\n", init, i, getpid(), getpgid(0));
                            temp = 0;
                            while (1)
                            {
                                sleep(1);
                                printf("%s №%d: процесс жив %d.\n", init, i, temp++);
                            }
                        }
                        else
                        {
                            printf("%s был создан дочерний процесс №%d, его pid: %d, его группа: %d.\n", init, i, getpid(), getpgid(0));
                            new_child = 0;
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
            init = CHILD3;

            sigemptyset(&sigset);
            sigaddset(&sigset, SIGINT);
            sigprocmask(SIG_BLOCK, &sigset, 0);
            if (!sigwait(&sigset, &temp))
            {
                system("ps -x");
                printf("%s я ухожу.\n", init);
            }
        }
    }
    return 0;
}
