#include <stdio.h>
#include <stdlib.h>
#include <errno.h> 
#include <unistd.h> 
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h> 

#define FTW_F 1    // файл, не являющийся каталогом
#define FTW_D 2    // каталог
#define FTW_DNR 3  // каталог, недоступный для чтения
#define FTW_NS 4   // файл, информацию о котором нельзя получить с помощью stat


static int printer(const char *pathame, const struct stat *statptr, int type)
{
	switch(type)
	{
		case FTW_F: 
			printf( "-- %s [%lu]\n", pathame, statptr->st_ino);
			break;
		case FTW_D: 
			printf( "-- %s/ [%lu]\n", pathame, statptr->st_ino);
			break;
		case FTW_DNR:
			perror("К одному из каталогов закрыт доступ."); 
			return -1;
		case FTW_NS:
			perror("Ошибка функции stat."); 
			return -1;
		default: 
			perror("Неизвестый тип файла."); 
			return -1;
	}
	return 0;
}

static int dopath(const char *filename, int depth)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	int ret = 0;

	if (ret = lstat(filename, &statbuf) < 0) // ошибка 
	{
		switch(ret)
		{
			case EBADF:
				printf("Неверный файловый описатель filedes.\n");
				break;
			case ENOENT:
				printf("Компонент полного имени файла file_name не существует или полное имя является пустой строкой. \n");
				break;
			case ENOTDIR:
				printf("Компонент пути не является каталогом. \n");
				break;
			case ELOOP:
				printf("При поиске файла встретилось слишком много символьных ссылок. \n");
				break;
			case EFAULT:
				printf("Некорректный адрес. \n");
				break;
			case EACCES:
				printf("Запрещен доступ. \n");
				break;
			case ENOMEM:
				printf("Недостаточно памяти в системе. \n");
				break;
			case ENAMETOOLONG:
				printf("Слишком длинное название файла. \n");
				break;
			default:
				printf("Неизвестная ошибка. \n");
				break;
		}
		return -1; 
	}

	for (int i = 0; i < depth; ++i)
		printf("|\t");

	if (S_ISDIR(statbuf.st_mode) == 0)  // не каталог 
		return printer(filename, &statbuf, FTW_F);

	if ((ret = printer(filename, &statbuf, FTW_D)) != 0)  // printer вернет ошибку
		return ret;

	if ((dp = opendir(filename)) == NULL)  // каталог недоступен
		return printer(filename, &statbuf, FTW_DNR);
    
	chdir(filename);
	while ((dirp = readdir(dp)) != NULL && ret == 0)
	{
        // пропуск каталогов . и ..
		if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 )
			ret = dopath(dirp->d_name, depth + 1);
	}
    
	chdir("..");

	if (closedir(dp) < 0)
		perror("Невозможно закрыть каталог");

	return ret;    
}


int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		printf("Вы должны указать директорию\n");
		return -1;
	}

	return dopath(argv[1], 0);
}