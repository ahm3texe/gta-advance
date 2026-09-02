import DecompDashboard, {
  type DashboardData,
} from '@/components/decomp-dashboard';
import decompData from './decomp-data.json';

export default function Home() {
  return <DecompDashboard data={decompData as unknown as DashboardData} />;
}
