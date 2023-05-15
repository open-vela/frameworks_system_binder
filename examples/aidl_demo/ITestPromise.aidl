import ITestPromiseCallback;

interface ITestPromise
{
    oneway void add(int a, int b, ITestPromiseCallback callback);
}
