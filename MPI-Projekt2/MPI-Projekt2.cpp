#include <mpi.h>
#include <iostream>
#include <vector>
#include <random>

using namespace std;

int fibonacci(int n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return fibonacci(n - 2) + fibonacci(n - 1);
}

void mergeArray(int arr[], int l, int m, int r)
{
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = new int[n1], *R = new int[n2];

    for (auto i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (auto j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = 1;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}


void mergeSort(int arr[], int l, int r) {
    if (l >= r) {
        return;
    }
    int m = l + (r - l) / 2;
    mergeSort(arr, l, m);
    mergeSort(arr, m + 1, r);
    mergeArray(arr, l, m, r);
}

void printArray(int A[], int size)
{
    for (int i = 0; i < size; i++)
        cout << A[i] << " ";
}


#define RESULT_TAG 1
#define TASK_TAG 2

int empty_task() { return -1; }

bool is_empty_task(int task) { return -1 == task; }

void coordinate_computations(int processes) {
    cout << "Coord" << endl;
    uint32_t N = 20;
    vector<int> tasks(N);

    default_random_engine generator;
    uniform_int_distribution<int> distribution(10, 20);


    for (auto& el : tasks) {
        el = distribution(generator);
    }

    for (int p = 1; p < processes; ++p) {
        tasks.push_back(empty_task());
    }

    vector<int> proc_to_task_id(processes, -1); 
    vector<int> results(N);

    const auto par_start_time = MPI_Wtime();

    int task_id = 0;
    for (auto task : tasks) {

        MPI_Status status;
        int result;
        const int code = MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE,
            RESULT_TAG, MPI_COMM_WORLD, &status);
        if (code != MPI_SUCCESS) {
            MPI_Abort(MPI_COMM_WORLD, code);
        }

        const auto sender = status.MPI_SOURCE;
        const auto prev_task_id = proc_to_task_id[sender];
        if (prev_task_id != -1) {
            results[prev_task_id] = result;
        }

        proc_to_task_id[sender] = task_id;
        MPI_Send(&task, 1, MPI_INT, sender, TASK_TAG, MPI_COMM_WORLD);

        ++task_id;
    }
    const auto par_duration = (MPI_Wtime() - par_start_time);

    printf("[Master]: All computations have finished in %.2lf sec.\n",
        par_duration);
    printf("[Master]: Results:\n");
    for (int i = 0; i < N; ++i) {
        printf("%4d  --  %10d\n", tasks[i], results[i]);
    }

    vector<int> verify_results;
    const auto seq_start_time = MPI_Wtime();
    for (int i = 0; i < N; ++i) {
        auto task = tasks[i];
        verify_results.push_back(fibonacci(task));
    }
    const auto seq_duration = (MPI_Wtime() - seq_start_time);
    printf("[Master]: Results for sequential execution:\n");
    for (int i = 0; i < N; ++i) {
        printf("%4d  --  %10d\n", tasks[i], verify_results[i]);
    }
    printf("[Master]: Seq. computations have finished in %.2lf sec.\n",
        seq_duration);
    printf("[Master]: Speedup was: %.1lfx\n", seq_duration / par_duration);
}

void perform_computations(int my_rank) {
    cout << "Pref" << endl;
    const int Master = 0;
    int result = -1;

    while (true) {
        printf("[%d] sending result [%d] to master\n", my_rank, result);

        MPI_Send(&result, 1, MPI_INT, Master, RESULT_TAG, MPI_COMM_WORLD);

        int task;
        MPI_Status status;
        MPI_Recv(&task, 1, MPI_INT, Master, TASK_TAG, MPI_COMM_WORLD, &status);

        printf("[%d] received a task [%d] from master\n", my_rank, task);

        if (is_empty_task(task)) {
            break;
        }
        result = fibonacci(task);
    }
}

int main(int argc, char* argv[]) {
    // Inicjalizacja MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        cerr << "Cannot initialize MPI subsystem. Aborting.\n";
        abort();
    }

    vector<int> nums = { 3,10,5,6,7,8,9,0,5,3,14,5,67,8 };
    for (auto num : nums) {
        cout << num << endl;
    }

    int processes;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Koordynator zadañ
    if (rank == 0) {
        coordinate_computations(processes);
    }
    //Wykonuj¹cy zadania
    else {
        perform_computations(rank);
    }

    ////Sortowanie oscylacyjne
    //int arr[] = { 12, 11, 13, 5, 6, 7 };
    //int arr_size = sizeof(arr) / sizeof(arr[0]);

    //cout << "Given array is \n";
    //printArray(arr, arr_size);

    //mergeSort(arr, 0, arr_size - 1);

    //cout << "\nSorted array is \n";
    //printArray(arr, arr_size);

    MPI_Finalize();
    return 0;
}